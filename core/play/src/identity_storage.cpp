#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include "identity_storage.hpp"
#include "token_encoding.hpp"

#include <nlohmann/json.hpp>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <cerrno>
    #include <fcntl.h>
    #include <unistd.h>
#endif

#include <array>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    constexpr std::size_t MaximumArchiveBytes = 4 * 1024 * 1024;

    void requireFields(const Json &value, std::initializer_list<const char *> fields)
    {
        if (!value.is_object() || value.size() != fields.size()) throw std::invalid_argument("invalid identity archive fields");
        for (const auto field : fields)
        {
            if (!value.contains(field)) throw std::invalid_argument("missing identity archive field");
        }
    }

    std::string textField(const Json &value, const char *name)
    {
        if (!value.at(name).is_string()) throw std::invalid_argument("identity archive field must be a string");
        return value.at(name).get<std::string>();
    }

    std::string_view roleName(Role role)
    {
        switch (role)
        {
            case Role::Any: throw std::invalid_argument("Any cannot be saved as a participant role");
            case Role::Spectator: return "spectator";
            case Role::Player: return "player";
            case Role::Master: return "master";
            case Role::Host: return "host";
        }
        throw std::invalid_argument("unknown saved role");
    }

    Role parseRole(std::string_view value)
    {
        if (value == "spectator") return Role::Spectator;
        if (value == "player") return Role::Player;
        if (value == "master") return Role::Master;
        if (value == "host") return Role::Host;
        throw std::invalid_argument("unknown saved role");
    }

    Json parseDocument(const std::string &document)
    {
        if (document.starts_with("\xef\xbb\xbf")) throw std::invalid_argument("identity archive must not contain a BOM");
        std::vector<std::unordered_set<std::string>> keys;
        const auto callback = [&keys](int depth, Json::parse_event_t event, Json &value)
        {
            if (depth > 16) throw std::invalid_argument("identity archive nesting exceeded");
            if (event == Json::parse_event_t::object_start) keys.emplace_back();
            else if (event == Json::parse_event_t::object_end) keys.pop_back();
            else if (event == Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
            {
                throw std::invalid_argument("duplicate identity archive key");
            }
            return true;
        };
        try { return Json::parse(document, callback); }
        catch (const Json::exception &) { throw std::invalid_argument("invalid identity archive JSON"); }
    }

#if defined(_WIN32)
    using NativeFile = HANDLE;
    const NativeFile InvalidFile = INVALID_HANDLE_VALUE;
    int fileError() { return static_cast<int>(GetLastError()); }
    void closeFile(NativeFile file) { CloseHandle(file); }
#else
    using NativeFile = int;
    constexpr NativeFile InvalidFile = -1;
    int fileError() { return errno; }
    void closeFile(NativeFile file) { ::close(file); }
#endif

    [[noreturn]] void fail(const char *operation)
    {
        throw std::system_error(fileError(), std::system_category(), operation);
    }

    class File final
    {
    public:
        explicit File(NativeFile handle = InvalidFile) : handle_(handle) {}
        ~File() { if (handle_ != InvalidFile) closeFile(handle_); }
        File(const File &) = delete;
        File &operator=(const File &) = delete;
        File(File &&other) noexcept : handle_(std::exchange(other.handle_, InvalidFile)) {}
        File &operator=(File &&other) noexcept
        {
            if (this != &other)
            {
                if (handle_ != InvalidFile) closeFile(handle_);
                handle_ = std::exchange(other.handle_, InvalidFile);
            }
            return *this;
        }
        NativeFile get() const { return handle_; }
        void write(std::string_view document)
        {
            std::size_t offset = 0;
            while (offset < document.size())
            {
#if defined(_WIN32)
                DWORD written = 0;
                if (!WriteFile(handle_, document.data() + offset, static_cast<DWORD>(document.size() - offset), &written, nullptr)) fail("WriteFile");
#else
                const auto written = ::write(handle_, document.data() + offset, document.size() - offset);
                if (written < 0 && errno == EINTR) continue;
                if (written < 0) fail("write");
#endif
                if (written == 0) throw std::runtime_error("identity archive write made no progress");
                offset += static_cast<std::size_t>(written);
            }
        }
        void flush()
        {
#if defined(_WIN32)
            if (!FlushFileBuffers(handle_)) fail("FlushFileBuffers");
#else
            while (::fsync(handle_) != 0)
            {
                if (errno != EINTR) fail("fsync");
            }
#endif
        }
        void close()
        {
            const auto handle = std::exchange(handle_, InvalidFile);
#if defined(_WIN32)
            if (!CloseHandle(handle)) fail("CloseHandle");
#else
            if (::close(handle) != 0) fail("close");
#endif
        }
    private:
        NativeFile handle_;
    };

    struct TemporaryPath
    {
        std::filesystem::path path;
        ~TemporaryPath()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    };

    void replaceFile(const std::filesystem::path &path, const std::string &document)
    {
        if (path.filename().empty()) throw std::invalid_argument("identity archive path must name a file");
        TemporaryPath cleanup;
        File file;
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            auto temporary = path;
            temporary += ".tmp." + UserToken::generate().value();
#if defined(_WIN32)
            const auto handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == InvalidFile)
            {
                if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) continue;
                fail("CreateFileW");
            }
#else
            const auto handle = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
            if (handle == InvalidFile)
            {
                if (errno == EEXIST) continue;
                fail("open");
            }
#endif
            file = File(handle);
            cleanup.path = std::move(temporary);
            break;
        }
        if (file.get() == InvalidFile) throw std::runtime_error("cannot create identity archive temporary file");
        file.write(document);
        file.flush();
        file.close();
#if defined(_WIN32)
        if (!MoveFileExW(cleanup.path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) fail("MoveFileExW");
#else
        if (::rename(cleanup.path.c_str(), path.c_str()) != 0) fail("rename");
        const auto parent = path.parent_path().empty() ? std::filesystem::path(".") : path.parent_path();
        File directory(::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
        if (directory.get() == InvalidFile) fail("open directory");
        directory.flush();
#endif
    }
}

namespace golarion::play::detail
{
    IdentitySaveData readIdentityState(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("cannot open identity archive");
        std::string document;
        std::array<char, 4096> bytes{};
        while (file.read(bytes.data(), bytes.size()) || file.gcount() > 0)
        {
            if (document.size() + static_cast<std::size_t>(file.gcount()) > MaximumArchiveBytes) throw std::invalid_argument("identity archive size exceeded");
            document.append(bytes.data(), static_cast<std::size_t>(file.gcount()));
        }
        if (file.bad()) throw std::runtime_error("cannot read identity archive");
        const auto root = parseDocument(document);
        requireFields(root, {"version", "users", "tokens"});
        if (!root.at("version").is_number_integer() || root.at("version") != 1) throw std::invalid_argument("unsupported identity archive version");
        if (!root.at("users").is_array() || !root.at("tokens").is_array()) throw std::invalid_argument("identity archive collections must be arrays");
        IdentitySaveData result;
        std::unordered_set<UserId> users;
        for (const auto &entry : root.at("users"))
        {
            requireFields(entry, {"userId", "role"});
            auto userId = textField(entry, "userId");
            if (!users.insert(userId).second) throw std::invalid_argument("duplicate identity archive user");
            result.users.push_back({std::move(userId), parseRole(textField(entry, "role"))});
        }
        for (const auto &entry : root.at("tokens"))
        {
            requireFields(entry, {"userId", "hash"});
            auto userId = textField(entry, "userId");
            if (!users.contains(userId)) throw std::invalid_argument("token references an unknown user");
            const auto hash = decodeHex<32>(textField(entry, "hash"));
            if (!hash) throw std::invalid_argument("invalid token hash in identity archive");
            result.tokens.push_back({std::move(userId), *hash});
        }
        return result;
    }

    void writeIdentityState(const std::filesystem::path &path, const IdentitySaveData &data)
    {
        Json users = Json::array();
        Json tokens = Json::array();
        std::unordered_set<UserId> knownUsers;
        for (const auto &entry : data.users)
        {
            knownUsers.insert(entry.userId);
            users.push_back(Json{{"userId", entry.userId}, {"role", roleName(entry.role)}});
        }
        for (const auto &entry : data.tokens)
        {
            if (!knownUsers.contains(entry.userId)) throw std::invalid_argument("token references an unknown user");
            tokens.push_back(Json{{"userId", entry.userId}, {"hash", encodeHex(entry.tokenHash)}});
        }
        const auto document = Json{{"version", 1}, {"users", std::move(users)}, {"tokens", std::move(tokens)}}.dump(2);
        if (document.size() > MaximumArchiveBytes) throw std::invalid_argument("identity archive size exceeded");
        replaceFile(path, document);
    }
}
