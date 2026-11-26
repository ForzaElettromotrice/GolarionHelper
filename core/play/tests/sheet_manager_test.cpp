#include "golarion/play/sheet_manager.hpp"
#include "golarion/play/session_server.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>

int main()
{
    namespace fs = std::filesystem;
    using namespace golarion::play;
    const auto root = fs::temp_directory_path() / ("golarion-sheet-index-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 1;
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    const auto folder = root / "sheets";
    if (!SheetManager(root).namesById().empty() || fs::exists(folder)) return 1;
    fs::create_directory(folder);
    const auto index = folder / "index.json";
    std::ofstream(index) << R"([{"id":"a","name":"Mago"},{"id":"b","name":"Mago"}])";
    std::ofstream(folder / "a.json") << "not a valid sheet";
    const SheetManager manager(root);
    if (manager.namesById().size() != 2 || manager.namesById().at("a") != "Mago" || manager.namesById().at("b") != "Mago") return 1;
    for (const char *invalid : {"{", "{}", R"([{"id":"a"}])", R"([{"id":"a","name":" "}])",
                               R"([{"id":"a","name":"Uno"},{"id":"a","name":"Due"}])",
                               R"({"sheets":[],"folders":[{"id":"a","name":"A","parentId":"b"},{"id":"b","name":"B","parentId":"a"}]})",
                               R"({"sheets":[{"id":"a","name":"A","folderId":"missing"}],"folders":[]})",
                               R"({"sheets":[{"id":"a","name":"A"}],"folders":[{"id":"a","name":"B"}]})"})
    {
        std::ofstream(index) << invalid;
        try { SessionServer host("host", std::nullopt, root); }
        catch (const std::exception &) { continue; }
        return 1;
    }
    fs::remove(index);
    using Json = nlohmann::json;
    std::string id;
    {
        SessionServer host("host", std::nullopt, root);
        auto call = [&](const char *name, Json payload) { return host.sendCommand(name, std::move(payload)).get(); };
        for (const auto &payload : {Json{{"name", "  "}}, Json{{"name", 2}}, Json{{"name", "A"}, {"id", "external"}}})
        {
            try { call("sheets.create", payload); }
            catch (const ProtocolError &e) { if (e.code() == ProtocolErrorCode::InvalidRequest) continue; }
            return 1;
        }
        id = call("sheets.create", {{"name", " Mago "}}).at("id").get<std::string>();
        const auto other = call("sheets.create", {{"name", "Mago"}}).at("id").get<std::string>();
        if (id == other || SheetManager(root).namesById().size() != 2) return 1;
        const auto sheet = Json::parse(std::ifstream(folder / ("sheet-" + id + ".json")));
        if (sheet.at("identity").at("name") != "Mago") return 1;
        if (call("sheets.delete", {{"id", other}}).at("id") != other || fs::exists(folder / ("sheet-" + other + ".json"))) return 1;
    }
    {
        SessionServer host("host", std::nullopt, root);
        if (SheetManager(root).namesById().at(id) != "Mago") return 1;
        fs::rename(index, folder / "backup.json");
        fs::create_directory(index); // Block the atomic index replacement.
        try { host.sendCommand("sheets.delete", {{"id", id}}).get(); return 1; } catch (const std::exception &) {}
        if (!fs::is_regular_file(folder / ("sheet-" + id + ".json"))) return 1;
        fs::remove(index);
        fs::rename(folder / "backup.json", index);
        host.sendCommand("sheets.delete", {{"id", id}}).get();
        try { host.sendCommand("sheets.delete", {{"id", "../index"}}).get(); return 1; }
        catch (const ProtocolError &e) { if (e.code() != ProtocolErrorCode::InvalidRequest) return 1; }
        if (!SheetManager(root).namesById().empty() || fs::exists(folder / ("sheet-" + id + ".json"))) return 1;
    }
    std::string parent, child, sheet;
    {
        SessionServer host("host", std::nullopt, root);
        auto call = [&](const char *name, Json payload) { return host.sendCommand(name, std::move(payload)).get(); };
        parent = call("sheets.folders.create", {{"name", "PNG"}}).at("id");
        child = call("sheets.folders.create", {{"name", "Alleati"}, {"parentId", parent}}).at("id");
        sheet = call("sheets.create", {{"name", "Merisiel"}, {"folderId", child}}).at("id");
        const auto file = folder / ("sheet-" + sheet + ".json");
        const auto before = Json::parse(std::ifstream(file));
        auto rejects = [&](const char *name, Json payload) {
            try { call(name, std::move(payload)); }
            catch (const ProtocolError &e) { return e.code() == ProtocolErrorCode::InvalidRequest; }
            return false;
        };
        if (!rejects("sheets.folders.delete", {{"id", parent}}) || !rejects("sheets.folders.delete", {{"id", child}})
            || !rejects("sheets.move", {{"id", parent}, {"folderId", child}}) || !rejects("sheets.move", {{"id", child}, {"folderId", child}})
            || !rejects("sheets.move", {{"id", child}, {"folderId", sheet}}) || !rejects("sheets.move", {{"id", sheet}})
            || !rejects("sheets.create", {{"name", "A"}, {"folderId", sheet}}) || !rejects("sheets.delete", {{"id", parent}})) return 1;
        call("sheets.move", {{"id", child}, {"folderId", nullptr}});
        call("sheets.move", {{"id", parent}, {"folderId", child}});
        call("sheets.move", {{"id", sheet}, {"folderId", parent}});
        if (Json::parse(std::ifstream(file)) != before) return 1;
    }
    {
        SessionServer host("host", std::nullopt, root);
        auto call = [&](const char *name, Json payload) { return host.sendCommand(name, std::move(payload)).get(); };
        for (const auto &occupied : {parent, child})
        {
            try { call("sheets.folders.delete", {{"id", occupied}}); return 1; }
            catch (const ProtocolError &e) { if (e.code() != ProtocolErrorCode::InvalidRequest) return 1; }
        }
        call("sheets.move", {{"id", sheet}, {"folderId", nullptr}});
        call("sheets.folders.delete", {{"id", parent}});
        call("sheets.folders.delete", {{"id", child}});
        if (SheetManager(root).namesById().at(sheet) != "Merisiel") return 1;
    }
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        SheetManager sheets({});
        sheets.registerCommands(protocol);
        const auto connection = protocol.registerConnection();
        for (const auto command : {"sheets.create", "sheets.delete", "sheets.move", "sheets.folders.create", "sheets.folders.delete"})
        {
            try { protocol.dispatch(command, protocol.makeContext(connection), Json::object()); return 1; }
            catch (const ProtocolError &e) { if (e.code() != ProtocolErrorCode::Forbidden) return 1; }
        }
        permissions.registerUser("alice");
        for (const auto flag : {Permission::Permission1, Permission::Permission2, Permission::Permission3})
            permissions.updatePermissions("host", "alice", PermissionSet{}, flag);
        protocol.bindUser(connection, "alice");
        const auto actor = protocol.makeContext(connection);
        const auto bag = protocol.dispatch("sheets.folders.create", actor, Json{{"name", "Giocatori"}});
        const auto created = protocol.dispatch("sheets.create", actor, Json{{"name", "Guerriero"}, {"folderId", bag.at("id")}});
        protocol.dispatch("sheets.move", actor, Json{{"id", created.at("id")}, {"folderId", nullptr}});
        protocol.dispatch("sheets.folders.delete", actor, Json{{"id", bag.at("id")}});
        protocol.dispatch("sheets.delete", actor, Json{{"id", created.at("id")}});
        if (!sheets.namesById().empty()) return 1;
    }
}
