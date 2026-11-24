#include "socket_support.hpp"
#include "protocol.hpp"
#include "golarion/play/session.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    using namespace std::chrono_literals;
    namespace fs = std::filesystem;

    void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }

    template<typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    template<typename T> T completed(std::future<T> result)
    {
        check(result.wait_for(3s) == std::future_status::ready, "permission command remained pending");
        return result.get();
    }

    void rejected(std::future<Json> result, ProtocolErrorCode expected)
    {
        check(result.wait_for(3s) == std::future_status::ready, "permission rejection remained pending");
        bool matched = false;
        try { result.get(); }
        catch (const ProtocolError &error) { matched = error.code() == expected; }
        check(matched, "wrong permission command rejection");
    }

    struct Directory
    {
        fs::path path = fs::absolute(fs::current_path()) / ("permission-commands-" + UserToken::generate().value());
        Directory() { check(fs::create_directory(path), "cannot create test directory"); }
        ~Directory()
        {
            std::error_code ignored;
            for (fs::directory_iterator it(path, ignored), end; it != end && !ignored; it.increment(ignored)) fs::remove(it->path(), ignored);
            fs::remove(path, ignored);
        }
    };

    std::string readText(const fs::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        check(static_cast<bool>(input), "cannot read permission archive");
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    std::uint64_t mask(Json result, const UserId &userId)
    {
        check(result.size() == 2 && result.at("userId") == userId && result.at("permissions").is_number_integer(), "unexpected permission result schema");
        return result.at("permissions").get<std::uint64_t>();
    }

    Json setPayload(const UserId &target, Permission permission, bool enabled)
    {
        return {{"targetUserId", target}, {"permission", static_cast<std::uint64_t>(permission)}, {"enabled", enabled}};
    }

    std::uint64_t savedMask(const fs::path &path, const UserId &userId)
    {
        const auto document = Json::parse(readText(path));
        check(document.at("version") == 4 && document.at("hostId") == "host", "permission command damaged archive metadata");
        for (const auto &user : document.at("users")) if (user.at("userId") == userId) return user.at("permissions").get<std::uint64_t>();
        throw std::runtime_error("permission command lost an archived user");
    }

    void registrationAndLocalPersistence()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        {
            auto host = Session::createHost("host");
            host.registerUser("alice");
            check(mask(completed(host.sendCommand("permissions.get", Json::object())), "host") == 63, "own read requires persistence");
            rejected(host.sendCommand("permissions.setPermission", setPayload("alice", Permission::Permission5, true)), ProtocolErrorCode::CommandFailed);
            check(mask(completed(host.sendCommand("permissions.get", {{"targetUserId", "alice"}})), "alice") == 7, "missing archive changed memory");
            host.saveIdentityState(archive);
            check(mask(completed(host.sendCommand("permissions.setPermission", setPayload("alice", Permission::Permission5, true))), "alice") == 23, "local mutation returned wrong mask");
            check(savedMask(archive, "alice") == 23, "local mutation did not autosave");
            host.stopHosting();
            check(mask(completed(host.sendCommand("permissions.applyPreset", {{"targetUserId", "alice"}, {"preset", "master"}})), "alice") == 31, "offline preset failed");
            check(savedMask(archive, "alice") == 31, "offline preset did not autosave");
            rejected(host.sendCommand("permissions.setPermission", setPayload("host", Permission::Permission1, true)), ProtocolErrorCode::Forbidden);
            rejected(host.sendCommand("permissions.applyPreset", {{"targetUserId", "host"}, {"preset", "master"}}), ProtocolErrorCode::Forbidden);
        }
        auto reopened = Session::createHost("host");
        reopened.loadIdentityState(archive);
        check(mask(completed(reopened.sendCommand("permissions.get", {{"targetUserId", "alice"}})), "alice") == 31, "local mutation was lost on restart");

        ResourceManager resources;
        resources.registerFeature<int()>("identity.updateUserPermissions", [] { return 9; });
        check(rejects([&] { ProtocolManager failed(resources); }), "identity service collision accepted");
        check(resources.invoke<int()>("identity.updateUserPermissions") == 9, "rollback removed another identity service");
        resources.registerFeature<void()>("protocol.add_command", [] {});
        resources.unregisterFeature("protocol.add_command");
        resources.unregisterFeature("identity.updateUserPermissions");
        {
            ProtocolManager protocol(resources);
            PermissionManager permissions(resources, "host");
            permissions.registerCommands();
            check(rejects([&] { permissions.registerCommands(); }), "duplicate permission command registration accepted");
            check(mask(protocol.dispatch("permissions.get", CommandContext{"host"}, Json::object()), "host") == 63, "duplicate registration replaced the callback");
        }
        resources.registerFeature<void()>("identity.updateUserPermissions", [] {});
        resources.unregisterFeature("identity.updateUserPermissions");
    }

    void strictPayloadValidation()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        auto host = Session::createHost("host");
        host.registerUser("alice");
        host.saveIdentityState(archive);
        const auto original = readText(archive);
        std::vector<Json> invalid;
        for (const auto &value : std::array<Json, 10>{0, 3, 64, -1, 16.0, true, "16", nullptr, Json::object(), Json::array()})
        {
            auto payload = setPayload("alice", Permission::Permission5, true);
            payload["permission"] = value;
            invalid.push_back(std::move(payload));
        }
        for (const auto &value : std::array<Json, 4>{0, "true", nullptr, Json::array()})
        {
            auto payload = setPayload("alice", Permission::Permission5, true);
            payload["enabled"] = value;
            invalid.push_back(std::move(payload));
        }
        invalid.push_back({{"targetUserId", "alice"}, {"permission", 16}});
        auto spoofed = setPayload("alice", Permission::Permission5, true);
        spoofed["actorId"] = "host";
        invalid.push_back(spoofed);
        invalid.push_back(setPayload("bad id", Permission::Permission5, true));
        invalid.push_back(setPayload("missing", Permission::Permission5, true));
        for (const auto &payload : invalid) rejected(host.sendCommand("permissions.setPermission", payload), ProtocolErrorCode::InvalidRequest);
        for (const auto &preset : std::array<Json, 6>{"Master", "unknown", 1, true, nullptr, Json::array()})
        {
            rejected(host.sendCommand("permissions.applyPreset", {{"targetUserId", "alice"}, {"preset", preset}}), ProtocolErrorCode::InvalidRequest);
        }
        rejected(host.sendCommand("permissions.get", {{"actorId", "host"}}), ProtocolErrorCode::InvalidRequest);
        rejected(host.sendCommand("permissions.get", {{"targetUserId", ""}}), ProtocolErrorCode::InvalidRequest);
        rejected(host.sendCommand("permissions.get", {{"targetUserId", 1}}), ProtocolErrorCode::InvalidRequest);
        rejected(host.sendCommand("permissions.get", {{"targetUserId", "missing"}}), ProtocolErrorCode::InvalidRequest);
        for (const auto &payload : std::vector<Json>{
            {{"targetUserId", "alice"}, {"grant", 3}, {"revoke", 2}},
            {{"targetUserId", "alice"}, {"grant", 64}, {"revoke", 0}},
            {{"targetUserId", "alice"}, {"grant", 1}, {"revoke", -1}},
            {{"targetUserId", "alice"}, {"grant", 1}, {"revoke", false}},
            {{"targetUserId", "alice"}, {"grant", 1}},
            {{"targetUserId", "alice"}, {"grant", 1}, {"revoke", 0}, {"actorId", "host"}}
        })
        {
            rejected(host.sendCommand("permissions.update", payload), ProtocolErrorCode::InvalidRequest);
        }
        check(readText(archive) == original && savedMask(archive, "alice") == 7, "invalid command changed persistent state");
    }

    void pendingPeerCannotReadPermissions(std::uint16_t port)
    {
        detail::SocketRuntime runtime;
        detail::Socket peer(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        check(peer.get() != detail::InvalidSocket, "pending socket failed");
        const auto address = detail::ipv4Address("127.0.0.1", port);
        check(::connect(peer.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "pending connect failed");
        detail::configureSocket(peer.get());
        const auto request = detail::encodeMessage({"permissions.get", "pending", Json::object()});
        const int sent = static_cast<int>(::send(peer.get(), reinterpret_cast<const char *>(request.data()), static_cast<int>(request.size()), 0));
        check(sent == static_cast<int>(request.size()), "pending request was not sent");
        std::vector<std::uint8_t> input;
        const auto deadline = std::chrono::steady_clock::now() + 3s;
        while (std::chrono::steady_clock::now() < deadline)
        {
            std::array<std::uint8_t, 4096> bytes{};
            const int count = static_cast<int>(::recv(peer.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), 0));
            if (count < 0 && detail::retryable(detail::socketError())) { std::this_thread::sleep_for(5ms); continue; }
            check(count > 0, "pending peer closed without a rejection");
            input.insert(input.end(), bytes.begin(), bytes.begin() + count);
            if (const auto reply = detail::decodeMessage(input))
            {
                check(reply->type == "request.rejected" && reply->payload.at("code") == "FORBIDDEN", "empty-mask read became public");
                return;
            }
        }
        throw std::runtime_error("pending permission read timed out");
    }

    void networkPolicyAutosaveAndRollback()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        std::optional<Session> host(std::in_place, "host");
        host->saveIdentityState(archive);
        auto port = host->startHosting("127.0.0.1", 0);
        auto code = SessionCode(host->getSessionCode());
        pendingPeerCannotReadPermissions(port);
        auto delegate = Session::createClient();
        auto target = Session::createClient();
        auto player = Session::createClient();
        const auto a = completed(delegate.joinSession("127.0.0.1", port, code));
        const auto b = completed(target.joinSession("127.0.0.1", port, code));
        const auto c = completed(player.joinSession("127.0.0.1", port, code));
        const auto read = [&](Session &client, const UserId &id) { return client.sendCommand("permissions.get", {{"targetUserId", id}}); };
        check(mask(completed(delegate.sendCommand("permissions.get", Json::object())), a.userId) == 7, "own read failed");
        rejected(read(player, a.userId), ProtocolErrorCode::Forbidden);
        rejected(player.sendCommand("permissions.setPermission", setPayload(b.userId, Permission::Permission5, true)), ProtocolErrorCode::Forbidden);
        completed(host->sendCommand("permissions.setPermission", setPayload(a.userId, Permission::ChangePermissions, true)));
        check(mask(completed(read(delegate, a.userId)), a.userId) == 39 && savedMask(archive, a.userId) == 39, "delegation was not effective and saved");
        check(delegate.getClientState().state == ClientConnectionState::Connected, "permission synchronization disconnected the delegate");
        check(mask(completed(read(delegate, b.userId)), b.userId) == 7, "delegate cannot read another user's permissions");
        completed(host->sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "spectator"}}));
        completed(host->sendCommand("permissions.setPermission", setPayload(b.userId, Permission::Permission1, false)));
        check(mask(completed(target.sendCommand("permissions.get", Json::object())), b.userId) == 0, "authenticated zero-mask user cannot read its permissions");
        check(mask(completed(delegate.sendCommand("permissions.setPermission", setPayload(b.userId, Permission::Permission5, true))), b.userId) == 16, "delegate mutation failed");
        completed(delegate.sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "master"}}));
        check(savedMask(archive, b.userId) == 31, "remote preset was not persisted automatically");
        rejected(delegate.sendCommand("permissions.setPermission", setPayload(a.userId, Permission::Permission5, true)), ProtocolErrorCode::Forbidden);
        rejected(delegate.sendCommand("permissions.applyPreset", {{"targetUserId", a.userId}, {"preset", "master"}}), ProtocolErrorCode::Forbidden);
        rejected(delegate.sendCommand("permissions.setPermission", setPayload("host", Permission::Permission1, false)), ProtocolErrorCode::Forbidden);
        rejected(delegate.sendCommand("permissions.setPermission", setPayload(b.userId, Permission::ChangePermissions, true)), ProtocolErrorCode::Forbidden);
        rejected(delegate.sendCommand("permissions.setPermission", setPayload(b.userId, Permission::ChangePermissions, false)), ProtocolErrorCode::Forbidden);
        completed(host->sendCommand("permissions.setPermission", setPayload(b.userId, Permission::ChangePermissions, true)));
        rejected(target.sendCommand("permissions.setPermission", setPayload(c.userId, Permission::ChangePermissions, true)), ProtocolErrorCode::Forbidden);
        completed(delegate.sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "spectator"}}));
        check(savedMask(archive, b.userId) == 33, "preset erased the target's delegation");
        const auto update = [](Session &client, const UserId &id, int grant, int revoke)
        {
            return client.sendCommand("permissions.update", {{"targetUserId", id}, {"grant", grant}, {"revoke", revoke}});
        };
        auto delegateUpdate = update(delegate, b.userId, 6, 1);
        auto hostUpdate = update(*host, b.userId, 16, 0);
        completed(std::move(delegateUpdate));
        completed(std::move(hostUpdate));
        check(savedMask(archive, b.userId) == 54, "batch update lost concurrent flags or the target's delegation");
        rejected(update(delegate, b.userId, 1, 32), ProtocolErrorCode::Forbidden);
        rejected(update(delegate, a.userId, 16, 1), ProtocolErrorCode::Forbidden);
        rejected(update(delegate, "host", 0, 1), ProtocolErrorCode::Forbidden);
        rejected(update(player, b.userId, 1, 16), ProtocolErrorCode::Forbidden);
        check(savedMask(archive, b.userId) == 54, "rejected batch wrote part of the change");
        check(mask(completed(update(*host, b.userId, 1, 22)), b.userId) == 33, "batch returned the wrong effective mask");

        const auto backup = directory.path / "before-failed-write.json";
        const auto original = readText(archive);
        fs::rename(archive, backup);
        check(fs::create_directory(archive), "cannot block archive replacement");
        rejected(delegate.sendCommand("permissions.setPermission", setPayload(b.userId, Permission::Permission5, true)), ProtocolErrorCode::CommandFailed);
        rejected(host->sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "master"}}), ProtocolErrorCode::CommandFailed);
        rejected(update(*host, b.userId, 18, 33), ProtocolErrorCode::CommandFailed);
        check(mask(completed(read(delegate, b.userId)), b.userId) == 33 && readText(backup) == original, "failed storage changed memory or the original archive");
        check(fs::is_empty(archive), "failed write left an unexpected file in the blocked archive directory");
        fs::remove(archive);
        fs::rename(backup, archive);
        check(readText(archive) == original, "restoring the test archive changed its bytes");

        completed(host->sendCommand("permissions.setPermission", setPayload(a.userId, Permission::ChangePermissions, false)));
        const auto afterRevocation = readText(archive);
        rejected(delegate.sendCommand("permissions.applyPreset", {{"targetUserId", c.userId}, {"preset", "master"}}), ProtocolErrorCode::Forbidden);
        check(readText(archive) == afterRevocation, "denied command wrote to the archive");
        completed(host->sendCommand("permissions.setPermission", setPayload(a.userId, Permission::ChangePermissions, true)));
        completed(host->sendCommand("permissions.applyPreset", {{"targetUserId", c.userId}, {"preset", "spectator"}}));
        completed(host->sendCommand("permissions.setPermission", setPayload(c.userId, Permission::Permission1, false)));
        std::vector<std::future<Json>> changes;
        changes.push_back(delegate.sendCommand("permissions.setPermission", setPayload(c.userId, Permission::Permission1, true)));
        changes.push_back(delegate.sendCommand("permissions.setPermission", setPayload(c.userId, Permission::Permission2, true)));
        for (const auto permission : {Permission::Permission3, Permission::Permission4, Permission::Permission5}) changes.push_back(host->sendCommand("permissions.setPermission", setPayload(c.userId, permission, true)));
        for (auto &change : changes) completed(std::move(change));
        check(savedMask(archive, c.userId) == 31, "concurrent local/remote transactions lost flags");
        const auto saved = Json::parse(readText(archive));
        check(saved.at("tokens").size() == 3, "permission updates changed token membership");
        host.reset();
        host.emplace("host");
        host->loadIdentityState(archive);
        port = host->startHosting("127.0.0.1", 0);
        code = SessionCode(host->getSessionCode());
        delegate.disconnectFromHost();
        target.disconnectFromHost();
        player.disconnectFromHost();
        const auto resumedA = completed(delegate.resumeSession("127.0.0.1", port, code, *a.userToken));
        const auto resumedB = completed(target.resumeSession("127.0.0.1", port, code, *b.userToken));
        const auto resumedC = completed(player.resumeSession("127.0.0.1", port, code, *c.userToken));
        check(resumedA.userId == a.userId && resumedA.permissions.toMask() == 39 && !resumedA.userToken, "delegate permissions or credential did not survive restart");
        check(resumedB.userId == b.userId && resumedB.permissions.toMask() == 33 && !resumedB.userToken, "target permissions or credential did not survive restart");
        check(resumedC.userId == c.userId && resumedC.permissions.toMask() == 31 && !resumedC.userToken, "concurrent permissions did not survive restart");
    }
}

int main()
{
    try
    {
        registrationAndLocalPersistence();
        strictPayloadValidation();
        networkPolicyAutosaveAndRollback();
        std::cout << "permission commands: PASS\n";
        return 0;
    }
    catch (const std::exception &error) { std::cerr << "permission commands: " << error.what() << '\n'; return 1; }
}
