#include "socket_support.hpp"
#include "protocol.hpp"
#include "golarion/play/session_client.hpp"
#include "golarion/play/session_server.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <set>
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
        check(result.wait_for(3s) == std::future_status::ready, "request did not complete");
        return result.get();
    }

    void rejected(std::future<Json> result, ProtocolErrorCode code)
    {
        check(result.wait_for(3s) == std::future_status::ready, "rejection did not complete");
        bool matched = false;
        try { result.get(); }
        catch (const ProtocolError &error) { matched = error.code() == code; }
        check(matched, "wrong command rejection");
    }

    struct Directory
    {
        std::filesystem::path path = std::filesystem::current_path() / ("send-test-" + UserToken::generate().value());
        Directory() { check(std::filesystem::create_directory(path), "cannot create test directory"); }
        ~Directory()
        {
            std::error_code error;
            for (std::filesystem::directory_iterator it(path, error), end; it != end && !error; it.increment(error)) std::filesystem::remove(it->path(), error);
            std::filesystem::remove(path, error);
        }
    };

    struct Host
    {
        ResourceManager resources;
        PermissionManager permissions{resources, "actual-host"};
        ServerProtocol protocol{resources};
    };

    void localCommandsAndFacade()
    {
        Host host;
        host.resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "actor", Permission::ChangePermissions, [](const auto &context, const auto &payload)
        {
            check(!context.connectionId, "local command invented a connection");
            return Json{{"actor", *context.userId}, {"value", payload.at("value")}};
        });
        check(completed(host.protocol.sendCommand(" actor ", {{"value", 4}, {"userId", "forged"}})) == Json{{"actor", "actual-host"}, {"value", 4}}, "local actor or result is wrong");
        rejected(host.protocol.sendCommand("unknown", Json::object()), ProtocolErrorCode::UnknownCommand);
        check(rejects<ProtocolError>([&] { host.protocol.sendCommand("actor", Json::array()); }), "non-object local payload accepted");
        host.protocol.add_command("nested", Permission::Permission2, [&](const auto &context, const auto &)
        {
            check(rejects<std::logic_error>([&] { host.protocol.sendCommand("actor", {{"value", 0}}); }), "callback could queue work as the host");
            return host.protocol.dispatch("actor", context, {{"value", 9}});
        });
        check(completed(host.protocol.sendCommand("nested", Json::object())).at("value") == 9, "nested dispatch stopped working");
        host.protocol.stopHosting();
        check(completed(host.protocol.sendCommand("actor", {{"value", 7}})).at("value") == 7, "stop disabled offline local commands");

        std::vector<std::future<Json>> pending;
        {
            SessionServer session("another-host");
            for (int i = 0; i < 24; ++i) pending.push_back(session.sendCommand("unknown", Json::object()));
        }
        for (auto &result : pending)
        {
            check(result.wait_for(1s) == std::future_status::ready, "Session destruction left a local future pending");
            try { result.get(); check(false, "unknown command succeeded"); }
            catch (const ProtocolError &error) { check(error.code() == ProtocolErrorCode::UnknownCommand, "provider was destroyed before the command worker"); }
            catch (const std::system_error &error) { check(error.code() == std::errc::operation_canceled, "wrong local cancellation"); }
        }
        SessionClient client;
        check(rejects<std::logic_error>([&] { client.sendCommand("actor", Json::object()); }), "disconnected client accepted a command");
    }

    void localQueueBoundsAndDestruction()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        auto protocol = std::make_unique<ServerProtocol>(resources);
        std::promise<void> started;
        std::promise<void> release;
        auto released = release.get_future().share();
        std::atomic<int> calls{0};
        protocol->add_command("block", Permission::ChangePermissions, [&](const auto &, const auto &)
        {
            ++calls;
            started.set_value();
            check(released.wait_for(4s) == std::future_status::ready, "local callback was not released");
            return Json(42);
        });
        auto running = protocol->sendCommand("block", Json::object());
        check(started.get_future().wait_for(2s) == std::future_status::ready, "local callback did not start");
        std::vector<std::future<Json>> queued;
        for (int i = 0; i < 64; ++i) queued.push_back(protocol->sendCommand("block", Json::object()));
        check(rejects<std::length_error>([&] { protocol->sendCommand("block", Json::object()); }), "local queue is unbounded");
        auto destroying = std::async(std::launch::async, [&] { protocol.reset(); });
        const bool canceled = queued.front().wait_for(1s) == std::future_status::ready;
        release.set_value();
        check(destroying.wait_for(2s) == std::future_status::ready, "local worker did not join");
        destroying.get();
        check(canceled && completed(std::move(running)) == 42 && calls == 1, "destruction executed queued callbacks or canceled active work");
        for (auto &result : queued)
        {
            check(rejects<std::system_error>([&] { completed(std::move(result)); }), "queued local command was not canceled");
        }
    }

    void realHostCommandsAndSerialization()
    {
        Directory directory;
        Host host;
        int active = 0;
        int sequence = 0;
        host.protocol.add_command("echo", Permission::Permission2, [&](const auto &context, const auto &payload)
        {
            check(++active == 1, "local and remote callbacks overlap");
            std::this_thread::sleep_for(1ms);
            const auto result = Json{{"actor", *context.userId}, {"value", payload.at("value")}, {"sequence", ++sequence}};
            --active;
            return result;
        });
        host.protocol.add_command("privileged", Permission::Permission4, [](const auto &, const auto &) { return Json::array({true, nullptr, 4}); });
        host.protocol.add_command("failure", Permission::Permission2, [](const auto &, const auto &) -> Json { throw std::runtime_error("private detail"); });
        host.protocol.add_command("null", Permission::Permission2, [](const auto &, const auto &) { return Json(nullptr); });
        host.protocol.add_command("no-flags", PermissionSet{}, [](const auto &context, const auto &) { return Json(*context.userId); });
        host.protocol.saveIdentityState(directory.path / "identities.json");
        const auto port = host.protocol.startHosting("127.0.0.1", 0);
        SessionClient client;
        const auto user = completed(client.joinSession("127.0.0.1", port, SessionCode(host.protocol.getSessionCode())));
        check(completed(client.sendCommand("echo", {{"value", 5}, {"userId", "actual-host"}, {"permissions", 63}})).at("actor") == user.userId, "payload changed the authenticated actor");
        rejected(client.sendCommand("privileged", Json::object()), ProtocolErrorCode::Forbidden);
        host.permissions.applyPreset("actual-host", user.userId, PermissionPreset::Master);
        check(completed(client.sendCommand("privileged", Json::object())) == Json::array({true, nullptr, 4}), "current permissions or array result was lost");
        host.permissions.applyPreset("actual-host", user.userId, PermissionPreset::Player);
        rejected(client.sendCommand("privileged", Json::object()), ProtocolErrorCode::Forbidden);
        rejected(client.sendCommand("unknown", Json::object()), ProtocolErrorCode::UnknownCommand);
        rejected(client.sendCommand("failure", Json::object()), ProtocolErrorCode::CommandFailed);
        rejected(client.sendCommand("session.join", {{"sessionCode", host.protocol.getSessionCode()}}), ProtocolErrorCode::AlreadyAdmitted);
        check(completed(client.sendCommand("null", Json::object())).is_null(), "null command result was lost");
        for (const auto flag : {Permission::Permission1, Permission::Permission2, Permission::Permission3}) host.permissions.updatePermissions("actual-host", user.userId, PermissionSet{}, flag);
        check(completed(client.sendCommand("no-flags", Json::object())) == user.userId, "authenticated empty-permission user could not execute an empty requirement");
        rejected(client.sendCommand("echo", {{"value", 9}}), ProtocolErrorCode::Forbidden);
        host.protocol.saveIdentityState(directory.path / "identities.json");
        client.disconnectFromHost();
        const auto resumedEmpty = completed(client.resumeSession("127.0.0.1", port, SessionCode(host.protocol.getSessionCode()), *user.userToken));
        check(resumedEmpty.permissions == PermissionSet{} && client.getClientState().permissions == PermissionSet{}, "zero mask was mistaken for missing identity or public access");
        host.permissions.applyPreset("actual-host", user.userId, PermissionPreset::Player);
        check(rejects<ProtocolError>([&] { client.sendCommand("echo", Json::array()); }), "non-object client payload accepted");
        check(rejects<std::invalid_argument>([&] { client.sendCommand("echo", {{"value", std::string(65536, 'x')}}); }), "oversized command accepted");

        std::vector<std::future<Json>> requests;
        for (int i = 0; i < 20; ++i)
        {
            requests.push_back(client.sendCommand("echo", {{"value", i}}));
            requests.push_back(host.protocol.sendCommand("echo", {{"value", i}}));
        }
        std::set<int> seen;
        for (std::size_t i = 0; i < requests.size(); ++i)
        {
            const auto result = completed(std::move(requests[i]));
            check(result.at("value") == i / 2 && result.at("actor") == (i % 2 == 0 ? user.userId : "actual-host"), "response was assigned to another request");
            seen.insert(result.at("sequence").get<int>());
        }
        check(seen.size() == requests.size(), "callback state was copied or executions overlapped");
        std::vector<std::future<void>> senders;
        for (int sender = 0; sender < 8; ++sender)
        {
            senders.push_back(std::async(std::launch::async, [&, sender]
            {
                for (int item = 0; item < 4; ++item)
                {
                    const int value = sender * 4 + item;
                    check(completed(client.sendCommand("echo", {{"value", value}})).at("value") == value, "concurrent submission lost a response");
                }
            }));
        }
        for (auto &sender : senders) completed(std::move(sender));
        client.disconnectFromHost();
        check(rejects<std::logic_error>([&] { client.sendCommand("echo", {{"value", 1}}); }), "disconnected client reused the socket");
        host.protocol.stopHosting();
        check(completed(host.protocol.sendCommand("echo", {{"value", 77}})).at("value") == 77, "network stop stopped local commands");
    }

    void hostingStopWithLocalAndRemoteWork()
    {
        Directory directory;
        Host host;
        std::promise<void> entered;
        std::promise<void> release;
        auto released = release.get_future().share();
        int remoteCalls = 0;
        host.protocol.add_command("block", Permission::ChangePermissions, [&](const auto &, const auto &)
        {
            entered.set_value();
            check(released.wait_for(4s) == std::future_status::ready, "stop test did not release the local command");
            return Json(true);
        });
        host.protocol.add_command("remote", Permission::Permission2, [&](const auto &, const auto &) { return Json(++remoteCalls); });
        host.protocol.saveIdentityState(directory.path / "identities.json");
        const auto port = host.protocol.startHosting("127.0.0.1", 0);
        const SessionCode code(host.protocol.getSessionCode());
        SessionClient client;
        const auto user = completed(client.joinSession("127.0.0.1", port, code));
        auto local = host.protocol.sendCommand("block", Json::object());
        check(entered.get_future().wait_for(2s) == std::future_status::ready, "blocking command did not start");
        auto remote = client.sendCommand("remote", Json::object());
        std::this_thread::sleep_for(100ms);
        std::promise<void> stopping;
        auto stopped = std::async(std::launch::async, [&] { stopping.set_value(); host.protocol.stopHosting(); });
        stopping.get_future().wait();
        const auto status = stopped.wait_for(100ms);
        release.set_value();
        completed(std::move(stopped));
        check(status == std::future_status::timeout && completed(std::move(local)) == true, "stop failed to wait for executing local work");
        check(remoteCalls == 0, "stop executed a remote request waiting behind local work");
        check(rejects<std::system_error>([&] { completed(std::move(remote)); }), "host stop left a pending client request");
        check(completed(host.protocol.sendCommand("remote", Json::object())) == 1, "offline local execution failed after stop");
        host.protocol.startHosting("127.0.0.1", port);
        check(completed(client.resumeSession("127.0.0.1", port, code, *user.userToken)).userId == user.userId, "resume failed after stop");
        check(completed(client.sendCommand("remote", Json::object())) == 2, "new connection reused an old pending request");
    }

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

    detail::ProtocolMessage readMessage(const detail::Socket &socket, std::vector<std::uint8_t> &input)
    {
        const auto deadline = Clock::now() + 3s;
        while (true)
        {
            if (auto message = detail::decodeMessage(input)) return *message;
            check(ready(socket, false, deadline), "scripted host did not receive a request");
            std::array<std::uint8_t, 4096> bytes{};
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), 0));
            check(count > 0, "client closed while scripted host read a request");
            input.insert(input.end(), bytes.begin(), bytes.begin() + count);
        }
    }

    void writeBytes(const detail::Socket &socket, const std::vector<std::uint8_t> &bytes)
    {
        std::size_t sent = 0;
        while (sent < bytes.size())
        {
            check(ready(socket, true, Clock::now() + 2s), "scripted host cannot write");
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + sent), static_cast<int>(bytes.size() - sent), flags));
            check(count > 0, "scripted host write failed");
            sent += static_cast<std::size_t>(count);
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
        explicit ScriptedHost(Script script)
        {
            listener_ = detail::Socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
            check(listener_.get() != detail::InvalidSocket, "scripted socket failed");
            detail::configureSocket(listener_.get());
            auto address = detail::ipv4Address("127.0.0.1", 0);
            check(::bind(listener_.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "scripted bind failed");
            check(::listen(listener_.get(), 1) == 0, "scripted listen failed");
            detail::AddressLength size = sizeof(address);
            check(::getsockname(listener_.get(), reinterpret_cast<sockaddr *>(&address), &size) == 0, "scripted port failed");
            port_ = ntohs(address.sin_port);
            task_ = std::async(std::launch::async, [this, script = std::move(script)]
            {
                check(ready(listener_, false, Clock::now() + 2s), "scripted host received no client");
                detail::Socket peer(::accept(listener_.get(), nullptr, nullptr));
                check(peer.get() != detail::InvalidSocket, "scripted accept failed");
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

    void outOfOrderAndFragmentedReplies()
    {
        ScriptedHost server([](const auto &peer, auto &input, const auto &)
        {
            std::array<detail::ProtocolMessage, 3> requests;
            for (auto &request : requests) request = readMessage(peer, input);
            check(requests[0].requestId != requests[1].requestId && requests[1].requestId != requests[2].requestId, "request IDs were reused");
            std::vector<std::uint8_t> bytes;
            for (int i = 2; i >= 0; --i)
            {
                const auto reply = detail::encodeMessage(accepted(requests[i], requests[i].payload.at("value")));
                bytes.insert(bytes.end(), reply.begin(), reply.end());
            }
            writeBytes(peer, {bytes.begin(), bytes.begin() + 2});
            std::this_thread::sleep_for(10ms);
            writeBytes(peer, {bytes.begin() + 2, bytes.end()});
        });
        SessionClient client;
        completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
        const std::array<Json, 3> values{Json(nullptr), Json::array({1, "x"}), Json("text")};
        std::vector<std::future<Json>> requests;
        for (const auto &value : values) requests.push_back(client.sendCommand("echo", {{"value", value}}));
        for (std::size_t i = 0; i < values.size(); ++i) check(completed(std::move(requests[i])) == values[i], "out-of-order reply lost correlation");
        server.finish();
    }

    void malformedFollowingReplyPreservesCompletedResults()
    {
        for (int variant = 0; variant < 6; ++variant)
        {
            ScriptedHost server([variant](const auto &peer, auto &input, const auto &)
            {
                const auto first = readMessage(peer, input);
                const auto second = readMessage(peer, input);
                auto good = detail::encodeMessage(accepted(first, 12));
                auto bad = accepted(second, 99);
                if (variant == 0) bad.requestId = "unknown";
                if (variant == 1) bad.requestId.reset();
                if (variant == 2) bad.type = "unsupported.event";
                if (variant == 3) { bad.type = "request.rejected"; bad.payload = {{"code", "FUTURE_CODE"}, {"message", "invalid"}}; }
                if (variant == 4) bad.requestId = first.requestId;
                if (variant == 5) bad.payload["extra"] = true;
                const auto bytes = detail::encodeMessage(bad);
                good.insert(good.end(), bytes.begin(), bytes.end());
                writeBytes(peer, good);
            });
            SessionClient client;
            completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
            auto first = client.sendCommand("echo", Json::object());
            auto second = client.sendCommand("echo", Json::object());
            check(completed(std::move(first)) == 12, "later invalid data discarded a completed response");
            rejected(std::move(second), ProtocolErrorCode::InvalidRequest);
            check(client.getClientState().state == ClientConnectionState::Disconnected, "malformed response kept the connection active");
            server.finish();
        }
    }

    void pendingCancellationTimeoutAndBounds()
    {
        for (int action = 0; action < 4; ++action)
        {
            std::promise<void> received;
            ScriptedHost server([&](const auto &peer, auto &input, const auto &stop)
            {
                readMessage(peer, input);
                received.set_value();
                if (action == 3) return; // EOF while requests are pending.
                const auto deadline = Clock::now() + 8s;
                while (!stop && Clock::now() < deadline) std::this_thread::sleep_for(5ms);
            });
            auto client = std::make_unique<SessionClient>();
            completed(client->joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
            std::vector<std::future<Json>> requests;
            for (int i = 0; i < (action == 3 ? 1 : 64); ++i) requests.push_back(client->sendCommand("unanswered", Json::object()));
            if (action != 3) check(rejects<std::length_error>([&] { client->sendCommand("overflow", Json::object()); }), "client outstanding requests are unbounded");
            check(received.get_future().wait_for(2s) == std::future_status::ready, "client did not send the pending request");
            if (action == 0) client->disconnectFromHost();
            if (action == 1) client.reset();
            const auto expected = action == 2 ? std::errc::timed_out : (action == 3 ? std::errc::connection_reset : std::errc::operation_canceled);
            check(requests.front().wait_for(6s) == std::future_status::ready, "pending commands have no bounded completion");
            for (auto &request : requests)
            {
                check(request.wait_for(1s) == std::future_status::ready, "cleanup left a pending command");
                bool matched = false;
                try { request.get(); }
                catch (const std::system_error &error) { matched = error.code() == expected; }
                check(matched, "wrong pending-command cancellation or timeout");
            }
            server.finish();
        }
    }
}

int main()
{
    try
    {
        localCommandsAndFacade();
        localQueueBoundsAndDestruction();
        realHostCommandsAndSerialization();
        hostingStopWithLocalAndRemoteWork();
        outOfOrderAndFragmentedReplies();
        malformedFollowingReplyPreservesCompletedResults();
        pendingCancellationTimeoutAndBounds();
        std::cout << "sendCommand: PASS\n";
        return 0;
    }
    catch (const std::exception &error) { std::cerr << "sendCommand: " << error.what() << '\n'; return 1; }
}
