#include "golarion/play/sheet_manager.hpp"
#include "atomic_file.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/play/server_protocol.hpp"
#include "golarion/play/user_token.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace golarion::play
{
    namespace fs = std::filesystem;
    using Json = nlohmann::json;

    namespace
    {
        bool validId(const std::string &id)
        {
            return !id.empty() && id.size() <= 128 && id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == id.npos;
        }

        void checkFolder(const fs::path &folder)
        {
            if (fs::is_symlink(folder)) throw std::runtime_error("Invalid campaign sheet directory");
        }

        std::string field(const Json &payload, const char *key)
        {
            if (!payload.contains(key) || !payload.at(key).is_string())
                throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Expected a string field");
            return payload.at(key).get<std::string>();
        }

        std::string destination(const Json &payload, const char *key)
        {
            if (!payload.contains(key) || payload.at(key).is_null()) return {};
            auto id = field(payload, key);
            if (!validId(id)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Invalid folder ID");
            return id;
        }
    }

    SheetManager::~SheetManager() = default;

    SheetManager::SheetManager(const std::filesystem::path &campaignDirectory)
    {
        if (campaignDirectory.empty()) return; // In-memory host, without a campaign archive.
        folder_ = campaignDirectory / "sheets";
        checkFolder(folder_);
        const auto path = folder_ / "index.json";
        if (fs::is_symlink(path)) throw std::runtime_error("Invalid campaign sheet index");
        if (!std::filesystem::exists(path)) return;
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open campaign sheet index");
        const auto index = nlohmann::json::parse(file);
        auto readEntries = [&](const Json &entries, Kind kind)
        {
            if (!entries.is_array()) throw std::invalid_argument("Invalid campaign index entries");
            auto &names = kind == Kind::Sheet ? index_.sheetNames : index_.folderNames;
            const auto key = kind == Kind::Sheet ? "folderId" : "parentId";
            for (const auto &entry : entries)
            {
                const auto id = entry.at("id").get<std::string>();
                const auto name = entry.at("name").get<std::string>();
                if (!validId(id) || name.find_first_not_of(" \t\r\n") == std::string::npos)
                    throw std::invalid_argument("Invalid campaign entry ID or name");
                if (index_.folderNames.contains(id) || index_.sheetNames.contains(id))
                    throw std::invalid_argument("Duplicate campaign entry ID: " + id);
                names.emplace(id, name);
                if (entry.contains(key) && !entry.at(key).is_null())
                    index_.parents.emplace(id, entry.at(key).get<std::string>());
            }
        };
        if (index.is_array()) readEntries(index, Kind::Sheet); // Original flat index.
        else
        {
            readEntries(index.at("folders"), Kind::Folder);
            readEntries(index.at("sheets"), Kind::Sheet);
        }
        std::set<std::string> verified;
        for (const auto &[id, parent] : index_.parents)
        {
            if (!index_.folderNames.contains(parent)) throw std::invalid_argument("Unknown parent folder");
            std::set<std::string> path;
            auto node = id;
            while (index_.parents.contains(node) && !verified.contains(node))
            {
                if (!path.insert(node).second) throw std::invalid_argument("Cyclic campaign folder index");
                node = index_.parents.at(node);
            }
            verified.insert(path.begin(), path.end());
        }
    }

    void SheetManager::registerCommands(ServerProtocol &protocol)
    {
        auto creation = [this](Kind kind, const Json &payload) {
            const auto key = kind == Kind::Sheet ? "folderId" : "parentId";
            if (payload.size() != 1u + payload.contains(key)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Unexpected creation fields");
            auto name = field(payload, "name");
            const auto first = name.find_first_not_of(" \t\r\n");
            if (first == name.npos) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Sheet name must not be blank");
            name = name.substr(first, name.find_last_not_of(" \t\r\n") - first + 1);
            return Json{{"id", create(kind, name, destination(payload, key))}, {"name", name}};
        };
        auto deletion = [this](Kind kind, const Json &payload) {
            if (payload.size() != 1) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Expected only an ID");
            const auto id = field(payload, "id");
            remove(kind, id);
            return Json{{"id", id}};
        };
        protocol.add_command("sheets.create", Permission::Any, [creation](const CommandContext &, const Json &p) { return creation(Kind::Sheet, p); });
        protocol.add_command("sheets.delete", Permission::Any, [deletion](const CommandContext &, const Json &p) { return deletion(Kind::Sheet, p); });
        protocol.add_command("sheets.folders.create", Permission::Any, [creation](const CommandContext &, const Json &p) { return creation(Kind::Folder, p); });
        protocol.add_command("sheets.folders.delete", Permission::Any, [deletion](const CommandContext &, const Json &p) { return deletion(Kind::Folder, p); });
        protocol.add_command("sheets.move", Permission::Any, [this](const CommandContext &, const Json &p) {
            if (p.size() != 2 || !p.contains("folderId")) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Expected id and folderId");
            const auto id = field(p, "id");
            move(id, destination(p, "folderId"));
            return Json{{"id", id}, {"folderId", p.at("folderId")}};
        });
    }

    void SheetManager::writeIndex(const Index &data) const
    {
        if (folder_.empty()) return;
        checkFolder(folder_);
        auto entries = [&](const auto &names, const char *key) {
            auto list = Json::array();
            for (const auto &[id, name] : names)
            {
                Json entry{{"id", id}, {"name", name}};
                if (auto it = data.parents.find(id); it != data.parents.end()) entry[key] = it->second;
                list.push_back(std::move(entry));
            }
            return list;
        };
        const Json index{{"sheets", entries(data.sheetNames, "folderId")}, {"folders", entries(data.folderNames, "parentId")}};
        detail::writeAtomicFile(folder_ / "index.json", index.dump(2));
    }

    std::string SheetManager::create(Kind kind, const std::string &name, const std::string &parent)
    {
        if (!parent.empty() && !index_.folderNames.contains(parent)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Unknown folder ID");
        if (!folder_.empty())
        {
            checkFolder(folder_);
            fs::create_directories(folder_);
        }
        std::string id;
        do { id = UserToken::generate().value(); }
        while (index_.sheetNames.contains(id) || index_.folderNames.contains(id) || (!folder_.empty() && fs::exists(fs::symlink_status(folder_ / ("sheet-" + id + ".json")))));
        auto next = index_;
        if (!parent.empty()) next.parents.emplace(id, parent);
        if (kind == Kind::Folder)
        {
            next.folderNames.emplace(id, name);
            writeIndex(next);
            index_ = std::move(next);
            return id;
        }
        auto sheet = std::make_unique<CharacterSheet>();
        sheet->setName(name);
        next.sheetNames.emplace(id, name);
        loaded_.emplace(id, std::move(sheet));
        try
        {
            if (!folder_.empty())
            {
                const auto path = folder_ / ("sheet-" + id + ".json");
                detail::writeAtomicFile(path, persistence::toJson(loaded_.at(id)->toSaveData()));
                try { writeIndex(next); }
                catch (...) { fs::remove(path); throw; }
            }
        }
        catch (...) { loaded_.erase(id); throw; }
        index_ = std::move(next);
        return id;
    }

    void SheetManager::move(const std::string &id, const std::string &parent)
    {
        if (!index_.sheetNames.contains(id) && !index_.folderNames.contains(id)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Unknown entry ID");
        if (!parent.empty() && !index_.folderNames.contains(parent)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Destination must be a folder");
        for (auto node = parent; !node.empty();)
        {
            if (node == id) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Cannot move a folder into itself or a descendant");
            auto it = index_.parents.find(node);
            if (it == index_.parents.end()) break;
            node = it->second;
        }
        auto next = index_;
        if (parent.empty()) next.parents.erase(id);
        else next.parents[id] = parent;
        writeIndex(next);
        index_ = std::move(next);
    }

    void SheetManager::remove(Kind kind, const std::string &id)
    {
        auto next = index_;
        next.parents.erase(id);
        if (kind == Kind::Folder)
        {
            if (!next.folderNames.erase(id)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Unknown folder ID");
            for (const auto &entry : next.parents)
                if (entry.second == id) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Folder is not empty");
            writeIndex(next);
            index_ = std::move(next);
            return;
        }
        if (!next.sheetNames.erase(id)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "Unknown campaign sheet ID");
        if (!folder_.empty())
        {
            checkFolder(folder_);
            const auto path = folder_ / ("sheet-" + id + ".json");
            if (!fs::is_regular_file(fs::symlink_status(path))) throw std::runtime_error("Missing or invalid campaign sheet file");
            writeIndex(next);
            try
            {
                if (!fs::remove(path)) throw std::runtime_error("Cannot remove campaign sheet file");
            }
            catch (...) { writeIndex(index_); throw; }
        }
        index_ = std::move(next);
        loaded_.erase(id);
    }
}
