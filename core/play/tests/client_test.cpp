#include "socket_support.hpp"
#include "golarion/play/session_client.hpp"
#include "golarion/play/session_server.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    namespace fs = std::filesystem;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    template<typename Exception, typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const Exception &) { return true; }
        return false;
    }

    template<typename Predicate> void eventually(Predicate predicate, std::chrono::seconds budget = 2s)
    {
        const auto deadline = Clock::now() + budget;
        while (!predicate())
        {
            check(Clock::now() < deadline, "client state did not settle");
            std::this_thread::sleep_for(5ms);
        }
    }

    struct TestDirectory
    {
        fs::path path = fs::absolute(fs::current_path()) / ("client-test-" + UserToken::generate().value());
        TestDirectory() { check(fs::create_directory(path), "cannot create test directory"); }
        ~TestDirectory()
        {
            std::error_code ignored;
            for (fs::directory_iterator iterator(path, ignored), end; iterator != end && !ignored; iterator.increment(ignored)) fs::remove(iterator->path(), ignored);
            fs::remove(path, ignored);
        }
    };

    struct Host
    {
        ResourceManager resources;
        ServerProtocol protocol{resources};
        PermissionManager permissions{resources, "host"};
        ~Host() { try { protocol.stopHosting(); } catch (...) { } }
    };

    ClientAdmission admitted(std::future<ClientAdmission> result)
    {
        check(result.wait_for(2s) == std::future_status::ready, "client admission did not complete");
        return result.get();
    }

    void checkRejected(std::future<ClientAdmission> result, ProtocolErrorCode expected)
    {
        check(result.wait_for(2s) == std::future_status::ready, "rejected admission did not complete");
        bool matched = false;
        try { static_cast<void>(result.get()); }
        catch (const ProtocolError &error) { matched = error.code() == expected; }
        check(matched, "wrong client rejection");
    }

    void modesAndProviderLifetime()
    {
        ResourceManager resources;
        ServerProtocol host(resources);
        PermissionManager permissions(resources, "host");
        {
            ClientProtocol peer(resources);
            check(rejects<std::invalid_argument>([&] { ClientProtocol collision(resources); }), "duplicate event provider accepted");
            ResourceManager clientResources;
            ClientProtocol client(clientResources);
            check(rejects<ProtocolError>([&] { client.dispatchEvent("session.join", CommandContext{"host"}, Json::object()); }), "client dispatched a server command");
        }
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "test", Permission::ChangePermissions, [](const auto &, const auto &) { return Json(true); });
        check(host.dispatch("test", CommandContext{"host"}, Json::object()) == true, "client teardown removed another provider's registration");
        ResourceManager isolated;
        ClientProtocol client(isolated);
        isolated.invoke<ClientProtocol::OnEventSignature>("protocol.on_event", "example.changed", [](const auto &, const auto &) {});
        isolated.registerFeature<void()>("permissions.getPermissions", [] {});
    }

    void realHostJoinResumeAndRestart()
    {
        TestDirectory directory;
        Host host;
        host.protocol.saveIdentityState(directory.path / "identities.json");
        const auto port = host.protocol.startHosting("127.0.0.1", 0);
        const SessionCode code(host.protocol.getSessionCode());
        SessionClient first;
        SessionClient second;
        auto joining = first.joinSession("127.0.0.1", port, code);
        check(joining.valid(), "join returned no future");
        const auto user = admitted(std::move(joining));
        check(user.userToken && user.permissions == permissionsForPreset(PermissionPreset::Player), "join did not return a Player and original token");
        check(host.protocol.resolveUserToken(user.userToken->value()) == user.userId, "server does not recognize client token");
        auto snapshot = first.getClientState();
        check(snapshot.state == ClientConnectionState::Connected && snapshot.userId == user.userId && snapshot.permissions == permissionsForPreset(PermissionPreset::Player), "admitted snapshot is wrong");
        snapshot.userId = "forged";
        check(first.getClientState().userId == user.userId, "snapshot aliases client state");
        check(rejects<std::logic_error>([&] { first.joinSession("127.0.0.1", port, code); }), "duplicate join replaced an active connection");
        const auto other = admitted(second.joinSession("127.0.0.1", port, code));
        check(other.userId != user.userId && other.userToken->value() != user.userToken->value(), "two clients share an identity");
        host.permissions.applyPreset("host", user.userId, PermissionPreset::Master);
        SessionClient replacement;
        const auto resumed = admitted(replacement.resumeSession("127.0.0.1", port, code, *user.userToken));
        check(resumed.userId == user.userId && resumed.permissions == permissionsForPreset(PermissionPreset::Master) && !resumed.userToken, "resume changed identity, permissions or token policy");
        eventually([&] { return first.getClientState().state == ClientConnectionState::Disconnected; });
        check(!first.getClientState().userId && !first.getClientState().permissions, "replaced client retained active identity");
        first.disconnectFromHost();
        second.disconnectFromHost();
        host.protocol.stopHosting();
        eventually([&] { return replacement.getClientState().state == ClientConnectionState::Disconnected; });
        host.protocol.startHosting("127.0.0.1", port);
        check(admitted(replacement.resumeSession("127.0.0.1", port, code, *user.userToken)).userId == user.userId, "client could not resume after host restart");
        replacement.disconnectFromHost();
        replacement.disconnectFromHost();
        check(host.permissions.toSaveData().size() == 3, "disconnect or resume changed registered participants");
    }

    void sessionFacadeAndIdentityArchive()
    {
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        SessionClient client;
        std::optional<UserToken> token;
        UserId userId;
        {
            SessionServer host("host");
            host.saveIdentityState(archive);
            const auto port = host.startHosting("127.0.0.1", 0);
            const auto user = admitted(client.joinSession("127.0.0.1", port, SessionCode(host.getSessionCode())));
            token = user.userToken;
            userId = user.userId;
        }
        eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
        SessionServer reopened("host");
        reopened.loadIdentityState(archive);
        const auto port = reopened.startHosting("127.0.0.1", 0);
        check(admitted(client.resumeSession("127.0.0.1", port, SessionCode(reopened.getSessionCode()), *token)).userId == userId, "reconstructed Session lost client identity");
        client.disconnectFromHost();
    }

    void admissionFailuresAndRetry()
    {
        TestDirectory directory;
        Host host;
        host.protocol.saveIdentityState(directory.path / "identities.json");
        const auto port = host.protocol.startHosting("127.0.0.1", 0);
        auto wrong = host.protocol.getSessionCode();
        wrong[0] = wrong[0] == 'A' ? 'B' : 'A';
        const SessionCode code(host.protocol.getSessionCode());
        SessionClient client;
        checkRejected(client.joinSession("127.0.0.1", port, SessionCode(wrong)), ProtocolErrorCode::InvalidSessionCode);
        check(client.getClientState().state == ClientConnectionState::Disconnected && host.permissions.toSaveData().size() == 1, "failed join changed identities or state");
        const auto user = admitted(client.joinSession("127.0.0.1", port, code));
        auto unknown = UserToken("0000000000000000");
        if (host.protocol.resolveUserToken(unknown.value())) unknown = UserToken("1111111111111111");
        SessionClient returning;
        checkRejected(returning.resumeSession("127.0.0.1", port, code, unknown), ProtocolErrorCode::InvalidUserToken);
        check(client.getClientState().state == ClientConnectionState::Connected && host.permissions.toSaveData().size() == 2, "invalid resume replaced the original client or created another user");
        check(admitted(returning.resumeSession("127.0.0.1", port, code, *user.userToken)).userId == user.userId, "failed resume prevented retry");
        check(rejects<std::invalid_argument>([&] { SessionCode invalid("bad"); }), "invalid code accepted");
        returning.disconnectFromHost();
        check(rejects<std::invalid_argument>([&] { returning.joinSession("localhost", port, code); }), "non-numeric address accepted");
        check(rejects<std::invalid_argument>([&] { returning.joinSession("127.0.0.1", 0, code); }), "zero client port accepted");
    }

    bool socketReady(detail::NativeSocket socket, bool writing, Clock::time_point deadline)
    {
        while (Clock::now() < deadline)
        {
            fd_set ready;
            FD_ZERO(&ready);
            FD_SET(socket, &ready);
            timeval timeout{0, 50000};
#if defined(_WIN32)
            const int count = ::select(0, writing ? nullptr : &ready, writing ? &ready : nullptr, nullptr, &timeout);
#else
            const int count = ::select(socket + 1, writing ? nullptr : &ready, writing ? &ready : nullptr, nullptr, &timeout);
#endif
            if (count > 0) return true;
            if (count < 0 && !detail::retryable(detail::socketError())) detail::fail("test select");
        }
        return false;
    }

    std::vector<std::uint8_t> readExact(const detail::Socket &socket, std::size_t size)
    {
        std::vector<std::uint8_t> bytes(size);
        std::size_t read = 0;
        const auto deadline = Clock::now() + 2s;
        while (read < size)
        {
            check(socketReady(socket.get(), false, deadline), "scripted host timed out reading");
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data() + read), static_cast<int>(size - read), 0));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "client closed before sending admission");
            read += static_cast<std::size_t>(count);
        }
        return bytes;
    }

    void writeBytes(const detail::Socket &socket, const std::vector<std::uint8_t> &bytes)
    {
        std::size_t sent = 0;
        const auto deadline = Clock::now() + 2s;
        while (sent < bytes.size())
        {
            check(socketReady(socket.get(), true, deadline), "scripted host timed out writing");
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + sent), static_cast<int>(bytes.size() - sent), flags));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "cannot send scripted response");
            sent += static_cast<std::size_t>(count);
        }
    }

    std::vector<std::uint8_t> rawFrame(std::string_view text)
    {
        const auto size = static_cast<std::uint32_t>(text.size());
        std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(size >> 24), static_cast<std::uint8_t>(size >> 16), static_cast<std::uint8_t>(size >> 8), static_cast<std::uint8_t>(size)};
        bytes.insert(bytes.end(), text.begin(), text.end());
        return bytes;
    }

    std::vector<std::uint8_t> frame(const Json &message) { return rawFrame(message.dump()); }

    std::string replaced(std::string text, std::string_view before, std::string_view after)
    {
        const auto position = text.find(before);
        check(position != std::string::npos, "wire mutation did not find its source field");
        text.replace(position, before.size(), after);
        return text;
    }

    Json accepted(const Json &request, bool withToken = true)
    {
        Json result{{"userId", "alice"}, {"permissions", 7}};
        if (withToken) result["userToken"] = "0123456789abcdef";
        return Json{{"version", 4}, {"type", "request.accepted"}, {"requestId", request.at("requestId")}, {"payload", {{"result", std::move(result)}}}};
    }

    class ScriptedHost final
    {
    public:
        using Script = std::function<void(const detail::Socket &, const Json &)>;
        explicit ScriptedHost(Script script) : listener_(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
        {
            check(listener_.get() != detail::InvalidSocket, "cannot create scripted listener");
            auto address = detail::ipv4Address("127.0.0.1", 0);
            check(::bind(listener_.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "cannot bind scripted host");
            check(::listen(listener_.get(), 1) == 0, "cannot listen with scripted host");
            detail::configureSocket(listener_.get());
            detail::AddressLength length = sizeof(address);
            check(getsockname(listener_.get(), reinterpret_cast<sockaddr *>(&address), &length) == 0, "missing scripted port");
            port_ = ntohs(address.sin_port);
            task_ = std::async(std::launch::async, [this, script = std::move(script)]
            {
                check(socketReady(listener_.get(), false, Clock::now() + 2s), "scripted host received no client");
                detail::Socket peer(::accept(listener_.get(), nullptr, nullptr));
                check(peer.get() != detail::InvalidSocket, "scripted accept failed");
                detail::configureSocket(peer.get());
                std::uint32_t size = 0;
                for (const auto byte : readExact(peer, 4)) size = (size << 8) | byte;
                check(size > 0 && size <= 65536, "client sent an invalid frame size");
                const auto document = readExact(peer, size);
                const auto request = Json::parse(document.begin(), document.end());
                check(request.at("version") == 4 && request.at("requestId").is_string(), "client sent an invalid envelope");
                script(peer, request);
                while (!stop_)
                {
                    if (!socketReady(peer.get(), false, Clock::now() + 50ms)) continue;
                    char byte;
                    const int count = static_cast<int>(::recv(peer.get(), &byte, 1, 0));
                    if (count <= 0) return;
                    throw std::runtime_error("client sent an unsolicited second request");
                }
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

    void admissionPrecedesUnexpectedFollowingData()
    {
        for (const int delivery : {0, 1, 2})
        {
            std::promise<void> release;
            auto released = release.get_future().share();
            ScriptedHost server([&](const auto &socket, const Json &request)
            {
                auto bytes = frame(accepted(request));
                const auto extra = frame(Json{{"version", 4}, {"type", "unsupported.event"}, {"payload", Json::object()}});
                if (delivery == 1)
                {
                    writeBytes(socket, bytes);
                    check(released.wait_for(3s) == std::future_status::ready, "following frame was not released");
                    writeBytes(socket, extra);
                }
                else
                {
                    if (delivery == 0) bytes.insert(bytes.end(), extra.begin(), extra.end());
                    else bytes.push_back(extra.front());
                    writeBytes(socket, bytes);
                    if (delivery == 2)
                    {
                        check(released.wait_for(3s) == std::future_status::ready, "partial following frame was not released");
                        writeBytes(socket, {extra.begin() + 1, extra.end()});
                    }
                }
            });
            SessionClient client;
            const auto result = admitted(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
            check(result.userId == "alice" && result.userToken && result.userToken->value() == "0123456789abcdef", "following data invalidated an already complete admission reply");
            if (delivery != 0) release.set_value();
            eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
            check(!client.getClientState().userId && !client.getClientState().permissions, "unsolicited data retained an active client identity");
            server.finish();
        }
    }

    void fragmentedReplyAndStrictValidation()
    {
        ScriptedHost server([](const auto &socket, const Json &request)
        {
            check(request.at("type") == "session.join" && request.at("payload") == Json{{"sessionCode", "A7K2Q"}, {"displayName", "Giocatore"}}, "client sent the wrong join payload");
            const auto bytes = frame(accepted(request));
            writeBytes(socket, {bytes.begin(), bytes.begin() + 2});
            std::this_thread::sleep_for(10ms);
            writeBytes(socket, {bytes.begin() + 2, bytes.end()});
        });
        SessionClient client;
        const auto user = admitted(client.joinSession("127.0.0.1", server.port(), SessionCode("a7k2q")));
        check(user.userId == "alice" && user.permissions == permissionsForPreset(PermissionPreset::Player) && user.userToken->value() == "0123456789abcdef", "fragmented response was corrupted");
        client.disconnectFromHost();
        server.finish();

        const std::vector<std::function<void(Json &)>> invalid{
            [](auto &reply) { reply["requestId"] = "wrong-id"; },
            [](auto &reply) { reply.erase("requestId"); },
            [](auto &reply) { reply["type"] = "session.joined"; },
            [](auto &reply) { reply["version"] = 1; },
            [](auto &reply) { reply["version"] = 2; },
            [](auto &reply) { reply["version"] = 3; },
            [](auto &reply) { reply["payload"]["extra"] = true; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = "any"; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = "administrator"; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = "7"; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = -1; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = 7.0; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = true; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = nullptr; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = Json::array(); },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = 64; },
            [](auto &reply) { reply["payload"]["result"]["permissions"] = std::numeric_limits<std::uint64_t>::max(); },
            [](auto &reply) { reply["payload"]["result"].erase("permissions"); },
            [](auto &reply) { reply["payload"]["result"]["userId"] = "bad id"; },
            [](auto &reply) { reply["payload"]["result"]["userId"] = ""; },
            [](auto &reply) { reply["payload"]["result"]["userToken"] = "ABCDEF0123456789"; },
            [](auto &reply) { reply["payload"]["result"].erase("userToken"); }
        };
        for (const auto &mutate : invalid)
        {
            ScriptedHost bad([&](const auto &socket, const Json &request) { auto reply = accepted(request); mutate(reply); writeBytes(socket, frame(reply)); });
            checkRejected(client.joinSession("127.0.0.1", bad.port(), SessionCode("A7K2Q")), ProtocolErrorCode::InvalidRequest);
            check(client.getClientState().state == ClientConnectionState::Disconnected, "invalid reply left client admitted");
            bad.finish();
        }
        ScriptedHost oversized([](const auto &socket, const Json &) { writeBytes(socket, {0, 1, 0, 1}); });
        checkRejected(client.joinSession("127.0.0.1", oversized.port(), SessionCode("A7K2Q")), ProtocolErrorCode::InvalidRequest);
        oversized.finish();
    }

    void malformedWireReplies()
    {
        const std::vector<std::function<std::string(const Json &)>> invalid{
            [](const auto &) { return std::string("{"); },
            [](const auto &) { return std::string("[]"); },
            [](const auto &request) { return std::string("\xef\xbb\xbf") + accepted(request).dump(); },
            [](const auto &request) { return accepted(request).dump() + " false"; },
            [](const auto &request) { return replaced(accepted(request).dump(), "alice", std::string(1, static_cast<char>(0xff))); },
            [](const auto &request) { return replaced(accepted(request).dump(), "\"version\":4", "\"version\":4,\"version\":4"); },
            [](const auto &request) { return replaced(accepted(request).dump(), "\"userId\":\"alice\"", "\"userId\":\"alice\",\"userId\":\"eve\""); },
            [](const auto &request) { return replaced(accepted(request).dump(), "alice", "a\\u0000b"); },
            [](const auto &request) { auto reply = accepted(request); reply["extra"] = true; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["version"] = 2.0; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["version"] = true; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["requestId"] = nullptr; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["payload"] = Json::array(); return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["payload"]["result"] = Json::array(); return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["payload"]["result"]["userId"] = 7; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["payload"]["result"]["permissions"] = nullptr; return reply.dump(); },
            [](const auto &request) { auto reply = accepted(request); reply["payload"]["result"]["userToken"] = false; return reply.dump(); },
            [](const auto &request) { return Json{{"version", 4}, {"type", "request.rejected"}, {"requestId", request.at("requestId")}, {"payload", {{"code", "FORBIDDEN"}, {"message", 7}}}}.dump(); },
            [](const auto &request) { return Json{{"version", 4}, {"type", "request.rejected"}, {"requestId", request.at("requestId")}, {"payload", {{"code", "FORBIDDEN"}, {"message", "Rifiutato."}, {"extra", true}}}}.dump(); },
            [](const auto &request)
            {
                auto reply = accepted(request);
                Json nested = nullptr;
                for (int depth = 0; depth < 65; ++depth) nested = Json::array({std::move(nested)});
                reply["payload"]["result"]["extra"] = std::move(nested);
                return reply.dump();
            }
        };
        SessionClient client;
        for (const auto &makeReply : invalid)
        {
            ScriptedHost server([&](const auto &socket, const Json &request) { writeBytes(socket, rawFrame(makeReply(request))); });
            checkRejected(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")), ProtocolErrorCode::InvalidRequest);
            check(client.getClientState().state == ClientConnectionState::Disconnected, "malformed wire data left an admitted client");
            server.finish();
        }
        ScriptedHost empty([](const auto &socket, const Json &) { writeBytes(socket, {0, 0, 0, 0}); });
        checkRejected(client.joinSession("127.0.0.1", empty.port(), SessionCode("A7K2Q")), ProtocolErrorCode::InvalidRequest);
        empty.finish();
    }

    void partialReplyEofAndMaximumFrame()
    {
        SessionClient client;
        for (const std::size_t cut : {std::size_t{2}, std::size_t{7}, std::size_t{120}})
        {
            std::promise<void> sent;
            auto seen = sent.get_future();
            ScriptedHost server([&](const auto &socket, const Json &request)
            {
                auto bytes = frame(accepted(request));
                check(cut < bytes.size(), "truncation did not leave a partial frame");
                bytes.resize(cut);
                writeBytes(socket, bytes);
                sent.set_value();
            });
            auto pending = client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q"));
            check(seen.wait_for(2s) == std::future_status::ready, "partial frame was not sent");
            server.finish();
            check(pending.wait_for(2s) == std::future_status::ready, "partial-frame EOF left a pending future");
            check(rejects<std::system_error>([&] { pending.get(); }), "partial-frame EOF was accepted or misclassified");
            check(client.getClientState().state == ClientConnectionState::Disconnected, "partial-frame EOF retained active state");
        }
        std::promise<std::size_t> expectedLength;
        auto expected = expectedLength.get_future();
        ScriptedHost maximum([&](const auto &socket, const Json &request)
        {
            auto reply = accepted(request);
            const auto length = 65536 - reply.dump().size() + 5;
            reply["payload"]["result"]["userId"] = std::string(length, 'u');
            const auto text = reply.dump();
            check(text.size() == 65536, "maximum frame was not constructed at the protocol boundary");
            expectedLength.set_value(length);
            writeBytes(socket, rawFrame(text));
        });
        const auto result = admitted(client.joinSession("127.0.0.1", maximum.port(), SessionCode("A7K2Q")));
        check(result.userId.size() == expected.get() && result.userToken && result.permissions == permissionsForPreset(PermissionPreset::Player), "maximum legal frame was rejected or truncated");
        client.disconnectFromHost();
        maximum.finish();
    }

    void partialProgressDoesNotResetAdmissionDeadline()
    {
        ScriptedHost server([](const auto &socket, const Json &request)
        {
            const auto bytes = frame(accepted(request));
            writeBytes(socket, {bytes.begin(), bytes.begin() + 4});
            for (std::size_t index = 4; index < 7; ++index)
            {
                std::this_thread::sleep_for(1200ms);
                writeBytes(socket, {bytes[index]});
            }
        });
        SessionClient client;
        auto pending = client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q"));
        check(pending.wait_for(6s) == std::future_status::ready, "partial input kept resetting the admission timeout");
        bool timedOut = false;
        try { pending.get(); }
        catch (const std::system_error &error) { timedOut = error.code() == std::errc::timed_out; }
        check(timedOut && client.getClientState().state == ClientConnectionState::Disconnected, "slow partial reply did not expire cleanly");
        server.finish();
    }

    void resumeWireContractAndRejections()
    {
        ScriptedHost server([](const auto &socket, const Json &request)
        {
            check(request.at("type") == "session.resume" && request.at("payload") == Json{{"sessionCode", "A7K2Q"}, {"userToken", "0123456789abcdef"}}, "client sent the wrong resume payload");
            auto reply = accepted(request, false);
            reply["payload"]["result"]["permissions"] = 1;
            writeBytes(socket, frame(reply));
        });
        SessionClient client;
        const auto user = admitted(client.resumeSession("127.0.0.1", server.port(), SessionCode("A7K2Q"), UserToken("0123456789abcdef")));
        check(!user.userToken && user.permissions == permissionsForPreset(PermissionPreset::Spectator), "resume issued another token or changed the permissions");
        client.disconnectFromHost();
        server.finish();
        for (const auto &[name, code] : std::vector<std::pair<const char *, ProtocolErrorCode>>{
            {"INVALID_REQUEST", ProtocolErrorCode::InvalidRequest}, {"INVALID_SESSION_CODE", ProtocolErrorCode::InvalidSessionCode},
            {"INVALID_USER_TOKEN", ProtocolErrorCode::InvalidUserToken}, {"NOT_ADMITTED", ProtocolErrorCode::NotAdmitted},
            {"ALREADY_ADMITTED", ProtocolErrorCode::AlreadyAdmitted}, {"ADMISSION_FAILED", ProtocolErrorCode::AdmissionFailed},
            {"UNKNOWN_COMMAND", ProtocolErrorCode::UnknownCommand}, {"FORBIDDEN", ProtocolErrorCode::Forbidden},
            {"COMMAND_FAILED", ProtocolErrorCode::CommandFailed}, {"UNKNOWN_ERROR", ProtocolErrorCode::InvalidRequest}})
        {
            ScriptedHost rejecting([&](const auto &socket, const Json &request) { writeBytes(socket, frame(Json{{"version", 4}, {"type", "request.rejected"}, {"requestId", request.at("requestId")}, {"payload", {{"code", name}, {"message", "Richiesta rifiutata."}}}})); });
            checkRejected(client.joinSession("127.0.0.1", rejecting.port(), SessionCode("A7K2Q")), code);
            rejecting.finish();
        }
        ScriptedHost rotated([](const auto &socket, const Json &request) { writeBytes(socket, frame(accepted(request, true))); });
        checkRejected(client.resumeSession("127.0.0.1", rotated.port(), SessionCode("A7K2Q"), UserToken("0123456789abcdef")), ProtocolErrorCode::InvalidRequest);
        rotated.finish();
    }

    void transportFailuresAndUnexpectedData()
    {
        detail::SocketRuntime runtime;
        detail::Socket reserved(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        check(reserved.get() != detail::InvalidSocket, "cannot reserve a test port");
        auto address = detail::ipv4Address("127.0.0.1", 0);
        check(::bind(reserved.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "cannot bind a test port");
        detail::AddressLength length = sizeof(address);
        check(getsockname(reserved.get(), reinterpret_cast<sockaddr *>(&address), &length) == 0, "cannot read reserved port");
        SessionClient client;
        auto refused = client.joinSession("127.0.0.1", ntohs(address.sin_port), SessionCode("A7K2Q"));
        check(refused.wait_for(6s) == std::future_status::ready, "connection refusal did not settle within the connect deadline");
        bool networkError = false;
        try { refused.get(); }
        catch (const std::system_error &error) { networkError = error.code() != std::errc::timed_out && error.code() != std::errc::operation_canceled; }
        check(networkError, "refused connection did not report the socket error");
        check(client.getClientState().state == ClientConnectionState::Disconnected, "refused connection stayed active");

        std::promise<void> received;
        auto seen = received.get_future();
        ScriptedHost closing([&](const auto &, const Json &) { received.set_value(); });
        auto pending = client.joinSession("127.0.0.1", closing.port(), SessionCode("A7K2Q"));
        check(seen.wait_for(2s) == std::future_status::ready, "closing host received no request");
        closing.finish();
        check(pending.wait_for(2s) == std::future_status::ready, "EOF left admission pending");
        check(rejects<std::system_error>([&] { pending.get(); }), "EOF before admission did not report a network error");

        std::promise<void> extra;
        auto sendExtra = extra.get_future().share();
        ScriptedHost unsolicited([&](const auto &socket, const Json &request)
        {
            writeBytes(socket, frame(accepted(request)));
            check(sendExtra.wait_for(3s) == std::future_status::ready, "unsolicited message was not released");
            writeBytes(socket, {'x', 0, 0, 0}); // Complete invalid length; a partial event prefix is now allowed.
        });
        static_cast<void>(admitted(client.joinSession("127.0.0.1", unsolicited.port(), SessionCode("A7K2Q"))));
        check(client.getClientState().state == ClientConnectionState::Connected, "valid admission did not connect");
        extra.set_value();
        eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
        check(!client.getClientState().userId && !client.getClientState().permissions, "protocol failure retained active identity");
        unsolicited.finish();
    }

    void simultaneousClientsAcrossReconstructedHosts()
    {
        constexpr std::size_t count = 8;
        constexpr std::size_t cycles = 3;
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        std::array<std::optional<ClientAdmission>, count> initial;
        std::array<std::array<std::optional<ClientAdmission>, count>, cycles> resumed;
        std::array<std::promise<void>, cycles> permit;
        std::array<std::shared_future<void>, cycles> ready;
        std::array<std::atomic<int>, cycles> returned{};
        for (std::size_t cycle = 0; cycle < cycles; ++cycle) ready[cycle] = permit[cycle].get_future().share();
        std::promise<void> finish;
        auto finished = finish.get_future().share();
        std::atomic<int> joined{0};
        std::optional<Host> host(std::in_place);
        host->protocol.saveIdentityState(archive);
        auto port = host->protocol.startHosting("127.0.0.1", 0);
        auto code = SessionCode(host->protocol.getSessionCode());
        std::vector<std::future<void>> clients;
        for (std::size_t index = 0; index < count; ++index)
        {
            clients.push_back(std::async(std::launch::async, [&, index]
            {
                SessionClient client;
                auto joining = client.joinSession("127.0.0.1", port, code);
                check(joining.wait_for(6s) == std::future_status::ready, "parallel join remained pending");
                initial[index] = joining.get();
                check(client.getClientState().state == ClientConnectionState::Connected, "parallel client did not stay connected");
                ++joined;
                for (std::size_t cycle = 0; cycle < cycles; ++cycle)
                {
                    check(ready[cycle].wait_for(8s) == std::future_status::ready, "host reconstruction was not released");
                    // The gate publishes the new endpoint/code; all previous attempts are complete.
                    client.disconnectFromHost();
                    auto returning = client.resumeSession("127.0.0.1", port, code, *initial[index]->userToken);
                    check(returning.wait_for(6s) == std::future_status::ready, "parallel resume remained pending");
                    resumed[cycle][index] = returning.get();
                    const auto expected = permissionsForPreset((index + cycle) % 2 == 0 ? PermissionPreset::Master : PermissionPreset::Spectator);
                    check(resumed[cycle][index]->userId == initial[index]->userId && resumed[cycle][index]->permissions == expected && !resumed[cycle][index]->userToken, "reconstructed host lost identity, permissions or original-token policy");
                    check(client.getClientState().permissions == expected, "parallel resume snapshot has a stale permissions");
                    ++returned[cycle];
                }
                check(finished.wait_for(8s) == std::future_status::ready, "parallel clients were not released");
            }));
        }
        eventually([&] { return joined.load() == static_cast<int>(count); }, 6s);
        std::set<UserId> identities;
        std::set<std::string> credentials;
        for (const auto &entry : initial)
        {
            check(entry && entry->userToken && entry->permissions == permissionsForPreset(PermissionPreset::Player), "parallel initial admission is incomplete");
            identities.insert(entry->userId);
            credentials.insert(entry->userToken->value());
        }
        check(identities.size() == count && credentials.size() == count, "parallel clients share an identity or token");
        for (std::size_t cycle = 0; cycle < cycles; ++cycle)
        {
            for (std::size_t index = 0; index < count; ++index) host->permissions.applyPreset("host", initial[index]->userId, (index + cycle) % 2 == 0 ? PermissionPreset::Master : PermissionPreset::Spectator);
            host->protocol.saveIdentityState(archive);
            host.reset();
            host.emplace();
            host->protocol.loadIdentityState(archive);
            port = host->protocol.startHosting("127.0.0.1", 0);
            code = SessionCode(host->protocol.getSessionCode());
            permit[cycle].set_value();
            eventually([&] { return returned[cycle].load() == static_cast<int>(count); }, 6s);
            check(host->permissions.toSaveData().size() == count + 1, "parallel resume duplicated participants");
            for (const auto &entry : initial) check(host->protocol.resolveUserToken(entry->userToken->value()) == entry->userId, "repeated reconstruction lost a credential");
        }
        finish.set_value();
        for (auto &client : clients) client.get();
    }

    void concurrentLifecycleCallsAndRapidCancellation()
    {
        TestDirectory directory;
        Host host;
        host.protocol.saveIdentityState(directory.path / "identities.json");
        const auto port = host.protocol.startHosting("127.0.0.1", 0);
        const SessionCode code(host.protocol.getSessionCode());
        SessionClient client;
        std::promise<void> release;
        auto start = release.get_future().share();
        std::atomic<int> rejected{0};
        auto attempt = [&]() -> std::optional<ClientAdmission>
        {
            check(start.wait_for(2s) == std::future_status::ready, "competing joins were not released");
            try { return admitted(client.joinSession("127.0.0.1", port, code)); }
            catch (const std::logic_error &) { ++rejected; return std::nullopt; }
        };
        auto first = std::async(std::launch::async, attempt);
        auto second = std::async(std::launch::async, attempt);
        release.set_value();
        const auto firstResult = first.get();
        const auto secondResult = second.get();
        check(firstResult.has_value() != secondResult.has_value() && rejected == 1, "concurrent starts did not preserve one active connection");
        check(host.permissions.toSaveData().size() == 2, "competing joins created two users");
        client.disconnectFromHost();
        for (int iteration = 0; iteration < 20; ++iteration)
        {
            auto pending = client.joinSession("127.0.0.1", port, code);
            auto stopFirst = std::async(std::launch::async, [&] { client.disconnectFromHost(); });
            auto stopSecond = std::async(std::launch::async, [&] { client.disconnectFromHost(); });
            check(stopFirst.wait_for(2s) == std::future_status::ready && stopSecond.wait_for(2s) == std::future_status::ready, "concurrent disconnect blocked");
            stopFirst.get();
            stopSecond.get();
            check(pending.wait_for(1s) == std::future_status::ready, "disconnect left an admission future pending");
            try
            {
                const auto result = pending.get();
                check(result.userToken && host.protocol.resolveUserToken(result.userToken->value()) == result.userId, "racing success returned an unrecognized identity");
            }
            catch (const std::system_error &error) { check(error.code() == std::errc::operation_canceled, "cancellation reported an unrelated transport error"); }
            const auto state = client.getClientState();
            check(state.state == ClientConnectionState::Disconnected && !state.userId && !state.permissions, "disconnect retained an old client state");
            const auto next = admitted(client.joinSession("127.0.0.1", port, code));
            check(next.userId == client.getClientState().userId && next.userToken, "cancellation corrupted the next connection");
            client.disconnectFromHost();
        }
    }

    void cancellationDestructionAndTimeout()
    {
        for (const bool destroy : {false, true})
        {
            std::promise<void> received;
            auto seen = received.get_future();
            std::promise<void> release;
            auto released = release.get_future().share();
            ScriptedHost stalled([&](const auto &, const Json &)
            {
                received.set_value();
                check(released.wait_for(3s) == std::future_status::ready, "stalled host was not released");
            });
            std::future<ClientAdmission> joining;
            const auto before = Clock::now();
            {
                SessionClient client;
                joining = client.joinSession("127.0.0.1", stalled.port(), SessionCode("A7K2Q"));
                check(seen.wait_for(2s) == std::future_status::ready, "client did not reach admission");
                check(client.getClientState().state == ClientConnectionState::Admitting, "wrong pending client state");
                check(rejects<std::logic_error>([&] { client.joinSession("127.0.0.1", stalled.port(), SessionCode("A7K2Q")); }), "overlapping admission accepted");
                if (!destroy) client.disconnectFromHost();
            }
            check(joining.wait_for(1s) == std::future_status::ready, "cancellation left the future pending");
            bool canceled = false;
            try { joining.get(); }
            catch (const std::system_error &error) { canceled = error.code() == std::errc::operation_canceled; }
            check(canceled && Clock::now() - before < 2s, "client cleanup failed to cancel promptly");
            release.set_value();
            stalled.finish();
        }
        std::promise<void> release;
        auto released = release.get_future().share();
        ScriptedHost stalled([&](const auto &, const Json &) { check(released.wait_for(7s) == std::future_status::ready, "timeout host was not released"); });
        SessionClient client;
        auto joining = client.joinSession("127.0.0.1", stalled.port(), SessionCode("A7K2Q"));
        check(joining.wait_for(6s) == std::future_status::ready, "admission has no bounded timeout");
        bool timedOut = false;
        try { joining.get(); }
        catch (const std::system_error &error) { timedOut = error.code() == std::errc::timed_out; }
        release.set_value();
        stalled.finish();
        check(timedOut && client.getClientState().state == ClientConnectionState::Disconnected, "timeout left an active connection");
    }
}

int main()
{
    try
    {
        modesAndProviderLifetime();
        admissionPrecedesUnexpectedFollowingData();
        realHostJoinResumeAndRestart();
        sessionFacadeAndIdentityArchive();
        admissionFailuresAndRetry();
        fragmentedReplyAndStrictValidation();
        malformedWireReplies();
        partialReplyEofAndMaximumFrame();
        partialProgressDoesNotResetAdmissionDeadline();
        resumeWireContractAndRejections();
        transportFailuresAndUnexpectedData();
        simultaneousClientsAcrossReconstructedHosts();
        concurrentLifecycleCallsAndRapidCancellation();
        cancellationDestructionAndTimeout();
        std::cout << "client: PASS\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "client: " << error.what() << '\n';
        return 1;
    }
}
