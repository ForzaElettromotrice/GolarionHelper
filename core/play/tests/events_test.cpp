#include "socket_support.hpp"
#include "protocol.hpp"
#include "golarion/play/session.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    using namespace golarion::play;
    using namespace std::chrono_literals;
    using Json = nlohmann::json;
    using Clock = std::chrono::steady_clock;

    void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

    template<typename Exception, typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const Exception &) { return true; }
        return false;
    }

    template<typename T> T completed(std::future<T> result)
    {
        check(result.wait_for(3s) == std::future_status::ready, "operation did not complete");
        return result.get();
    }

    template<typename Predicate> void eventually(Predicate predicate)
    {
        const auto deadline = Clock::now() + 3s;
        while (!predicate())
        {
            check(Clock::now() < deadline, "expected event/state did not arrive");
            std::this_thread::sleep_for(5ms);
        }
    }

    struct Directory
    {
        std::filesystem::path path = std::filesystem::current_path() / ("events-test-" + UserToken::generate().value());
        Directory() { check(std::filesystem::create_directory(path), "cannot create test directory"); }
        ~Directory() { std::error_code error; std::filesystem::remove_all(path, error); }
    };

    bool ready(const detail::Socket &socket, bool writing, Clock::time_point deadline)
    {
        while (Clock::now() < deadline)
        {
            fd_set descriptors;
            FD_ZERO(&descriptors);
            FD_SET(socket.get(), &descriptors);
            timeval timeout{0, 10000};
#if defined(_WIN32)
            const int selected = ::select(0, writing ? nullptr : &descriptors, writing ? &descriptors : nullptr, nullptr, &timeout);
#else
            const int selected = ::select(socket.get() + 1, writing ? nullptr : &descriptors, writing ? &descriptors : nullptr, nullptr, &timeout);
#endif
            if (selected > 0) return true;
            if (selected < 0 && !detail::retryable(detail::socketError())) detail::fail("select");
        }
        return false;
    }

    void writeBytes(const detail::Socket &socket, const std::vector<std::uint8_t> &bytes)
    {
        const auto deadline = Clock::now() + 3s;
        std::size_t sent = 0;
        while (sent < bytes.size())
        {
            check(ready(socket, true, deadline), "socket did not become writable");
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + sent), static_cast<int>(bytes.size() - sent), flags));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "socket write failed");
            sent += static_cast<std::size_t>(count);
        }
    }

    detail::ProtocolMessage readMessage(const detail::Socket &socket, std::vector<std::uint8_t> &input)
    {
        const auto deadline = Clock::now() + 3s;
        while (true)
        {
            if (auto message = detail::decodeMessage(input)) return *message;
            check(ready(socket, false, deadline), "socket did not receive a complete message");
            std::array<std::uint8_t, 4096> bytes{};
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), 0));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "socket closed before a complete message");
            input.insert(input.end(), bytes.begin(), bytes.begin() + count);
        }
    }

    detail::ProtocolMessage accepted(const detail::ProtocolMessage &request, Json result)
    {
        return {"request.accepted", request.requestId, {{"result", std::move(result)}}};
    }

    class ScriptedHost final
    {
    public:
        using Script = std::function<void(const detail::Socket &, std::vector<std::uint8_t> &, const std::atomic<bool> &)>;
        explicit ScriptedHost(Script script) : listener_(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
        {
            check(listener_.get() != detail::InvalidSocket, "cannot create listener");
            detail::configureSocket(listener_.get());
            auto address = detail::ipv4Address("127.0.0.1", 0);
            check(::bind(listener_.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "cannot bind listener");
            check(::listen(listener_.get(), 1) == 0, "cannot start listener");
            detail::AddressLength size = sizeof(address);
            check(getsockname(listener_.get(), reinterpret_cast<sockaddr *>(&address), &size) == 0, "cannot get listener port");
            port_ = ntohs(address.sin_port);
            task_ = std::async(std::launch::async, [this, script = std::move(script)]
            {
                check(ready(listener_, false, Clock::now() + 3s), "scripted host received no client");
                detail::Socket peer(::accept(listener_.get(), nullptr, nullptr));
                check(peer.get() != detail::InvalidSocket, "cannot accept client");
                detail::configureSocket(peer.get());
                std::vector<std::uint8_t> input;
                const auto request = readMessage(peer, input);
                writeBytes(peer, detail::encodeMessage(accepted(request, {{"userId", "alice"}, {"permissions", 7}, {"userToken", "0123456789abcdef"}})));
                script(peer, input, stop_);
            });
        }
        ~ScriptedHost() { stop_ = true; if (task_.valid()) task_.wait(); }
        std::uint16_t port() const { return port_; }
        void finish() { stop_ = true; task_.get(); }

    private:
        detail::SocketRuntime runtime_;
        detail::Socket listener_;
        std::uint16_t port_ = 0;
        std::atomic<bool> stop_{false};
        std::future<void> task_;
    };

    void registrationAndResourceLifetime()
    {
        ResourceManager resources;
        {
            ProtocolManager client(resources, SessionMode::Client);
            resources.invoke<ProtocolManager::AddClientCommandSignature>("protocol.add_command", "example.changed", [](const auto &, const auto &) { return Json(nullptr); });
            check(rejects<std::invalid_argument>([&] { client.add_command(" example.changed ", [](const auto &, const auto &) { return Json(nullptr); }); }), "duplicate notification replaced a callback");
            for (const auto name : {"", "  ", "permissions.changed", "request.accepted", " request.rejected "})
                check(rejects<std::invalid_argument>([&] { client.add_command(name, [](const auto &, const auto &) { return Json(nullptr); }); }), "invalid/reserved notification registered");
            check(rejects<std::invalid_argument>([&] { client.add_command("empty", {}); }), "empty callback registered");
            check(rejects<ProtocolError>([&] { client.dispatch("example.changed", {}, Json::object()); }), "notification dispatched without an admitted host");
            check(rejects<std::logic_error>([&] { client.sendEvent("alice", "example.changed", Json::object()); }), "client could send a host notification");
        }
        resources.registerFeature<void()>("protocol.add_command", [] {});
        resources.unregisterFeature("protocol.add_command");
        resources.registerFeature<int()>("protocol.sendEvent", [] { return 9; });
        check(rejects<std::invalid_argument>([&] { ProtocolManager collision(resources); }), "sendEvent provider collision accepted");
        check(resources.invoke<int()>("protocol.sendEvent") == 9, "rollback removed another provider");
        resources.registerFeature<void()>("protocol.add_command", [] {});
        resources.unregisterFeature("protocol.add_command");
        resources.unregisterFeature("protocol.sendEvent");
        ProtocolManager host(resources);
        check(!host.sendEvent("offline", "example.changed", Json::object()), "offline notification was queued");
        check(rejects<std::logic_error>([&] { host.add_command("client-only", [](const auto &, const auto &) { return Json(nullptr); }); }), "host accepted the client overload");
        for (const auto name : {"", "request.accepted", "request.rejected"})
            check(rejects<std::invalid_argument>([&] { host.sendEvent("alice", name, Json::object()); }), "invalid event name accepted");
        check(rejects<std::invalid_argument>([&] { host.sendEvent("alice", "example.changed", Json::array()); }), "non-object event payload accepted");
        check(rejects<std::invalid_argument>([&] { host.sendEvent("alice", "example.changed", {{"value", std::string(65536, 'x')}}); }), "oversized event accepted");
    }

    void targetedNotificationsAndSharedDispatch()
    {
        Directory directory;
        auto host = Session::createHost("host");
        host.saveIdentityState(directory.path / "identities.json");
        host.add_command("echo", PermissionSet{}, [&](const CommandContext &context, const Json &payload)
        {
            check(host.sendEvent(*context.userId, "example.changed", payload), "callback could not enqueue an event");
            return payload.at("value");
        });
        const auto port = host.startHosting("127.0.0.1", 0);
        const SessionCode code(host.getSessionCode());
        auto first = Session::createClient();
        auto second = Session::createClient();
        std::atomic<int> firstCount{0}, secondCount{0};
        CommandContext saved;
        first.add_command("example.changed", [&](const CommandContext &context, const Json &)
        {
            check(!context.userId && context.connectionId, "notification fabricated a participant sender");
            check(first.getClientState().state == ClientConnectionState::Connected, "callback cannot read the snapshot");
            check(rejects<std::logic_error>([&] { first.disconnectFromHost(); }), "callback joined its own worker");
            check(rejects<std::logic_error>([&] { first.sendCommand("echo", {{"value", 0}}); }), "callback could wait on its worker");
            saved = context;
            ++firstCount;
            return Json("ignored");
        });
        second.add_command("example.changed", [&](const auto &, const auto &) { ++secondCount; return Json(nullptr); });
        const auto a = completed(first.joinSession("127.0.0.1", port, code));
        completed(second.joinSession("127.0.0.1", port, code));
        check(host.sendEvent(a.userId, " example.changed ", {{"value", -1}}), "idle notification was rejected");
        eventually([&] { return firstCount == 1; });
        std::vector<std::future<Json>> pending;
        for (int value = 0; value < 12; ++value) pending.push_back(first.sendCommand("echo", {{"value", value}}));
        for (int value = 0; value < 12; ++value) check(completed(std::move(pending[value])) == value, "notification consumed a command reply");
        eventually([&] { return firstCount == 13; });
        check(secondCount == 0, "targeted notifications were broadcast");
        first.disconnectFromHost();
        const auto previous = firstCount.load();
        completed(first.resumeSession("127.0.0.1", port, code, *a.userToken));
        check(host.sendEvent(a.userId, "example.changed", {{"value", 99}}), "resumed client did not receive events");
        eventually([&] { return firstCount == previous + 1; });
        second.disconnectFromHost();
        first.disconnectFromHost();

        ResourceManager resources;
        ProtocolManager client(resources, SessionMode::Client);
        client.add_command("example.changed", [](const auto &, const auto &) { return Json(nullptr); });
        check(rejects<ProtocolError>([&] { client.dispatch("example.changed", saved, Json::object()); }), "a different client accepted a cached context");
    }

    void permissionSnapshotsAndTransactionalDelivery()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        auto host = Session::createHost("host");
        host.saveIdentityState(archive);
        host.registerUser("raw-user");
        const auto rawToken = host.issueUserToken("raw-user");
        const auto port = host.startHosting("127.0.0.1", 0);
        const SessionCode code(host.getSessionCode());
        auto delegate = Session::createClient();
        auto target = Session::createClient();
        const auto a = completed(delegate.joinSession("127.0.0.1", port, code));
        const auto b = completed(target.joinSession("127.0.0.1", port, code));
        const auto change = [](const UserId &userId, Permission flag, bool enabled)
        {
            return Json{{"targetUserId", userId}, {"permission", static_cast<std::uint64_t>(flag)}, {"enabled", enabled}};
        };
        completed(host.sendCommand("permissions.setPermission", change(a.userId, Permission::ChangePermissions, true)));
        eventually([&] { return delegate.getClientState().permissions == PermissionSet::fromMask(39); });
        completed(delegate.sendCommand("permissions.setPermission", change(b.userId, Permission::Permission5, true)));
        eventually([&] { return target.getClientState().permissions == PermissionSet::fromMask(23); });
        check(delegate.getClientState().permissions == PermissionSet::fromMask(39), "target change altered another client's snapshot");

        detail::SocketRuntime runtime;
        detail::Socket raw(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        auto address = detail::ipv4Address("127.0.0.1", port);
        check(::connect(raw.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "raw client could not connect");
        detail::configureSocket(raw.get());
        std::vector<std::uint8_t> input;
        writeBytes(raw, detail::encodeMessage({"session.resume", "raw-1", {{"sessionCode", code.value()}, {"userToken", rawToken}}}));
        check(readMessage(raw, input).type == "request.accepted", "raw client was not admitted");
        completed(host.sendCommand("permissions.setPermission", change("raw-user", Permission::Permission5, true)));
        const auto event = readMessage(raw, input);
        check(event.type == "permissions.changed" && !event.requestId && event.payload == Json{{"userId", "raw-user"}, {"permissions", 23}}, "wrong permission event envelope");
        completed(host.sendCommand("permissions.setPermission", change("raw-user", Permission::Permission5, true)));
        check(input.empty() && !ready(raw, false, Clock::now() + 100ms), "no-op emitted a permission event");

        const auto backup = directory.path / "backup.json";
        std::filesystem::rename(archive, backup);
        check(std::filesystem::create_directory(archive), "cannot block the archive");
        auto failed = host.sendCommand("permissions.setPermission", change("raw-user", Permission::Permission4, true));
        check(rejects<ProtocolError>([&] { completed(std::move(failed)); }), "failed persistence succeeded");
        check(input.empty() && !ready(raw, false, Clock::now() + 100ms), "failed save emitted an event");
        std::filesystem::remove(archive);
        std::filesystem::rename(backup, archive);
        completed(host.sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "spectator"}}));
        completed(host.sendCommand("permissions.setPermission", change(b.userId, Permission::Permission1, false)));
        eventually([&] { return target.getClientState().permissions == PermissionSet{}; });
        check(target.getClientState().state == ClientConnectionState::Connected, "zero permissions disconnected a client");
        target.disconnectFromHost();
        completed(host.sendCommand("permissions.applyPreset", {{"targetUserId", b.userId}, {"preset", "master"}}));
        const auto resumed = completed(target.resumeSession("127.0.0.1", port, code, *b.userToken));
        check(resumed.permissions == PermissionSet::fromMask(31), "offline mutation was not recovered by resume");
    }

    void fragmentedEventsInterleavedWithReplies()
    {
        std::promise<void> prefixSent, release;
        auto released = release.get_future().share();
        ScriptedHost server([&](const auto &peer, auto &input, const auto &stop)
        {
            auto event = detail::encodeMessage({"permissions.changed", std::nullopt, {{"userId", "alice"}, {"permissions", 23}}});
            writeBytes(peer, {event.begin(), event.begin() + 2});
            prefixSent.set_value();
            check(released.wait_for(3s) == std::future_status::ready, "fragmented event was not released");
            writeBytes(peer, {event.begin() + 2, event.end()});
            const auto first = readMessage(peer, input);
            const auto second = readMessage(peer, input);
            std::vector<std::uint8_t> bytes;
            for (const auto &message : std::vector<detail::ProtocolMessage>{accepted(second, 22), {"permissions.changed", std::nullopt, {{"userId", "alice"}, {"permissions", 0}}}, {"example.changed", std::nullopt, {{"value", 4}}}, accepted(first, 11)})
            {
                const auto frame = detail::encodeMessage(message);
                bytes.insert(bytes.end(), frame.begin(), frame.end());
            }
            writeBytes(peer, bytes);
            while (!stop) std::this_thread::sleep_for(5ms);
        });
        auto client = Session::createClient();
        std::atomic<int> calls{0};
        client.add_command("example.changed", [&](const auto &, const auto &payload) { check(payload.at("value") == 4, "wrong event payload"); ++calls; return Json(nullptr); });
        const auto admission = completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
        completed(prefixSent.get_future());
        std::this_thread::sleep_for(75ms);
        check(client.getClientState().state == ClientConnectionState::Connected, "partial idle event was treated as an unsolicited reply");
        release.set_value();
        eventually([&] { return client.getClientState().permissions == PermissionSet::fromMask(23); });
        auto first = client.sendCommand("echo", {{"value", 1}});
        auto second = client.sendCommand("echo", {{"value", 2}});
        check(completed(std::move(first)) == 11 && completed(std::move(second)) == 22, "interleaved event broke request correlation");
        eventually([&] { return calls == 1 && client.getClientState().permissions == PermissionSet{}; });
        check(admission.userToken && admission.userToken->value() == "0123456789abcdef", "event changed the admission credential");
        client.disconnectFromHost();
        server.finish();
    }

    void invalidEventsAndStaleContexts()
    {
        for (const auto &payload : std::vector<Json>{{{"userId", "other"}, {"permissions", 7}}, {{"userId", "alice"}, {"permissions", -1}}, {{"userId", "alice"}, {"permissions", 64}}, {{"userId", "alice"}, {"permissions", 1.0}}, {{"userId", "alice"}, {"permissions", true}}, {{"permissions", 7}}, {{"userId", "alice"}, {"permissions", 7}, {"extra", true}}})
        {
            ScriptedHost server([&](const auto &peer, auto &, const auto &stop)
            {
                writeBytes(peer, detail::encodeMessage({"permissions.changed", std::nullopt, payload}));
                while (!stop) std::this_thread::sleep_for(5ms);
            });
            auto client = Session::createClient();
            completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
            eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
            check(!client.getClientState().permissions, "invalid event left an active permission snapshot");
            server.finish();
        }

        Directory directory;
        auto host = Session::createHost("host");
        host.saveIdentityState(directory.path / "identities.json");
        const auto port = host.startHosting("127.0.0.1", 0);
        ResourceManager resources;
        ProtocolManager client(resources, SessionMode::Client);
        std::promise<CommandContext> received;
        client.add_command("capture", [&](const auto &context, const auto &)
        {
            check(client.dispatch("inspect", context, Json::object()) == client.getClientState().userId.value(), "nested client dispatch failed");
            received.set_value(context);
            return Json(nullptr);
        });
        client.add_command("inspect", [&](const auto &, const auto &) { return Json(client.getClientState().userId.value()); });
        const SessionCode code(host.getSessionCode());
        const auto user = completed(client.joinSession("127.0.0.1", port, code));
        host.sendEvent(user.userId, "capture", Json::object());
        const auto context = completed(received.get_future());
        check(client.dispatch("inspect", context, Json::object()) == user.userId, "common client dispatcher cannot invoke callbacks");
        check(rejects<ProtocolError>([&] { client.dispatch("inspect", CommandContext{"host", context.connectionId}, Json::object()); }), "client accepted a claimed participant sender");
        client.disconnectFromHost();
        check(rejects<ProtocolError>([&] { client.dispatch("inspect", context, Json::object()); }), "disconnected client accepted an old context");
        completed(client.resumeSession("127.0.0.1", port, code, *user.userToken));
        check(rejects<ProtocolError>([&] { client.dispatch("inspect", context, Json::object()); }), "resumed client accepted the previous connection context");
    }

    void boundedQueuesAndReplacement()
    {
        Directory directory;
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        ProtocolManager protocol(resources);
        protocol.saveIdentityState(directory.path / "identities.json");
        std::atomic<int> queued{0};
        protocol.add_command("flood", PermissionSet{}, [&](const CommandContext &context, const auto &)
        {
            for (int value = 0; value < 65; ++value)
                if (resources.invoke<ProtocolManager::SendEventSignature>("protocol.sendEvent", *context.userId, "example.changed", Json{{"value", value}})) ++queued;
            return Json(true);
        });
        protocol.add_command("flood-bytes", PermissionSet{}, [&](const CommandContext &context, const auto &)
        {
            for (int value = 0; value < 5; ++value)
                if (protocol.sendEvent(*context.userId, "example.changed", Json{{"value", std::string(65000, 'x')}})) ++queued;
            return Json(true);
        });
        const auto port = protocol.startHosting("127.0.0.1", 0);
        const SessionCode code(protocol.getSessionCode());
        auto client = Session::createClient();
        std::atomic<int> calls{0};
        client.add_command("example.changed", [&](const auto &, const auto &) { ++calls; return Json(nullptr); });
        const auto user = completed(client.joinSession("127.0.0.1", port, code));
        auto flood = client.sendCommand("flood", Json::object());
        check(rejects<std::exception>([&] { completed(std::move(flood)); }), "overflow did not disconnect the slow peer");
        check(queued == 64 && calls == 0, "event queue was unbounded or leaked discarded frames");
        completed(client.resumeSession("127.0.0.1", port, code, *user.userToken));
        queued = 0;
        auto byteFlood = client.sendCommand("flood-bytes", Json::object());
        check(rejects<std::exception>([&] { completed(std::move(byteFlood)); }), "byte overflow did not disconnect the slow peer");
        check(queued == 4 && calls == 0, "event queue did not bound its total bytes");
        auto replacement = Session::createClient();
        replacement.add_command("example.changed", [&](const auto &, const auto &) { ++calls; return Json(nullptr); });
        completed(replacement.resumeSession("127.0.0.1", port, code, *user.userToken));
        check(protocol.sendEvent(user.userId, "example.changed", Json::object()), "replacement did not receive new events");
        eventually([&] { return calls == 1; });
        protocol.stopHosting();
        check(!protocol.sendEvent(user.userId, "example.changed", Json::object()), "stop retained an active outbox");
        const auto restartedPort = protocol.startHosting("127.0.0.1", 0);
        completed(client.resumeSession("127.0.0.1", restartedPort, code, *user.userToken));
        check(protocol.sendEvent(user.userId, "example.changed", Json::object()), "restart did not enable events");
        eventually([&] { return calls == 2; });
    }

    void notificationsDoNotRenewCommandDeadlines()
    {
        ScriptedHost server([](const auto &peer, auto &input, const auto &stop)
        {
            static_cast<void>(readMessage(peer, input));
            for (int value = 0; value < 40 && !stop; ++value)
            {
                writeBytes(peer, detail::encodeMessage({"tick", std::nullopt, {{"value", value}}}));
                std::this_thread::sleep_for(100ms);
            }
            while (!stop) std::this_thread::sleep_for(5ms);
        });
        auto client = Session::createClient();
        std::atomic<int> calls{0};
        client.add_command("tick", [&](const auto &, const auto &) { ++calls; return Json(nullptr); });
        completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
        const auto before = Clock::now();
        auto request = client.sendCommand("never-replied", Json::object());
        check(request.wait_for(6s) == std::future_status::ready, "notifications renewed a command deadline");
        bool timedOut = false;
        try { request.get(); }
        catch (const std::system_error &error) { timedOut = error.code() == std::errc::timed_out; }
        check(timedOut && calls >= 20 && Clock::now() - before < 6s, "live traffic masked a pending command timeout");
        check(client.getClientState().state == ClientConnectionState::Disconnected, "timeout left the notification channel active");
        server.finish();
    }

    void concurrentStopAndCallbackSafety()
    {
        Directory directory;
        auto host = Session::createHost("host");
        host.saveIdentityState(directory.path / "identities.json");
        const auto port = host.startHosting("127.0.0.1", 0);
        auto client = Session::createClient();
        std::promise<void> entered, release;
        auto released = release.get_future().share();
        client.add_command("blocking", [&](const auto &, const auto &)
        {
            entered.set_value();
            check(released.wait_for(3s) == std::future_status::ready, "callback was not released");
            static_cast<void>(client.getClientState());
            check(rejects<std::logic_error>([&] { client.disconnectFromHost(); }), "callback could recursively disconnect");
            return Json(nullptr);
        });
        const auto user = completed(client.joinSession("127.0.0.1", port, SessionCode(host.getSessionCode())));
        host.sendEvent(user.userId, "blocking", Json::object());
        completed(entered.get_future());
        auto stopped = std::async(std::launch::async, [&] { client.disconnectFromHost(); });
        std::this_thread::sleep_for(75ms);
        release.set_value();
        completed(std::move(stopped));
        check(client.getClientState().state == ClientConnectionState::Disconnected, "disconnect left a running callback");

        auto returning = Session::createClient();
        completed(returning.resumeSession("127.0.0.1", port, SessionCode(host.getSessionCode()), *user.userToken));
        std::promise<void> localEntered, localRelease;
        auto localReleased = localRelease.get_future().share();
        host.add_command("stop-sender", Permission::ChangePermissions, [&](const auto &, const auto &)
        {
            localEntered.set_value();
            check(localReleased.wait_for(3s) == std::future_status::ready, "local callback was not released");
            return Json(host.sendEvent(user.userId, "permissions.changed", {{"userId", user.userId}, {"permissions", 7}}));
        });
        auto command = host.sendCommand("stop-sender", Json::object());
        completed(localEntered.get_future());
        auto hostStopped = std::async(std::launch::async, [&] { host.stopHosting(); });
        std::this_thread::sleep_for(100ms);
        localRelease.set_value();
        completed(std::move(hostStopped));
        check(completed(std::move(command)) == false, "callback enqueued an event after stop");
    }
}

int main()
{
    try
    {
        registrationAndResourceLifetime();
        targetedNotificationsAndSharedDispatch();
        permissionSnapshotsAndTransactionalDelivery();
        fragmentedEventsInterleavedWithReplies();
        invalidEventsAndStaleContexts();
        boundedQueuesAndReplacement();
        notificationsDoNotRenewCommandDeadlines();
        concurrentStopAndCallbackSafety();
        std::cout << "Live event tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Live event test failed: " << error.what() << '\n';
        return 1;
    }
}
