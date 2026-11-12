#if defined(_WIN32) && !defined(_WIN32_WINNT)
    #define _WIN32_WINNT 0x0601
#endif

#include "golarion/play/session.hpp"
#include "golarion/play/user_token.hpp"

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    namespace fs = std::filesystem;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

#if defined(_WIN32)
    using NativeSocket = SOCKET;
    constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
    int socketError() { return WSAGetLastError(); }
    void closeSocket(NativeSocket socket) { closesocket(socket); }
    bool disconnected(int error) { return error == WSAECONNRESET || error == WSAECONNABORTED; }
#else
    using NativeSocket = int;
    constexpr NativeSocket InvalidSocket = -1;
    int socketError() { return errno; }
    void closeSocket(NativeSocket socket) { ::close(socket); }
    bool disconnected(int error) { return error == ECONNRESET || error == ECONNABORTED; }
#endif

    [[noreturn]] void fail(const char *operation)
    {
        throw std::system_error(socketError(), std::system_category(), operation);
    }

    class Socket final
    {
    public:
        explicit Socket(NativeSocket handle) : handle_(handle) {}
        ~Socket() { if (handle_ != InvalidSocket) closeSocket(handle_); }
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket(Socket &&other) noexcept : handle_(std::exchange(other.handle_, InvalidSocket)) {}
        NativeSocket get() const { return handle_; }
    private:
        NativeSocket handle_;
    };

    Socket connectClient(std::uint16_t port)
    {
        Socket socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        if (socket.get() == InvalidSocket) fail("socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(socket.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0) fail("connect");
#if defined(_WIN32)
        const DWORD timeout = 2500;
#else
        const timeval timeout{2, 500000};
#endif
        for (const int option : {SO_RCVTIMEO, SO_SNDTIMEO})
        {
            if (setsockopt(socket.get(), SOL_SOCKET, option, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) != 0) fail("setsockopt");
        }
        return socket;
    }

    std::vector<std::uint8_t> frame(const Json &message)
    {
        const auto text = message.dump();
        const auto size = static_cast<std::uint32_t>(text.size());
        std::vector<std::uint8_t> result{static_cast<std::uint8_t>(size >> 24), static_cast<std::uint8_t>(size >> 16), static_cast<std::uint8_t>(size >> 8), static_cast<std::uint8_t>(size)};
        result.insert(result.end(), text.begin(), text.end());
        return result;
    }

    void sendBytes(const Socket &socket, const std::vector<std::uint8_t> &bytes)
    {
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + offset), static_cast<int>(bytes.size() - offset), flags));
            if (count <= 0) fail("send");
            offset += static_cast<std::size_t>(count);
        }
    }

    std::vector<std::uint8_t> receiveExact(const Socket &socket, std::size_t size)
    {
        std::vector<std::uint8_t> bytes(size);
        std::size_t offset = 0;
        while (offset < size)
        {
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data() + offset), static_cast<int>(size - offset), 0));
            check(count > 0, "missing response bytes");
            offset += static_cast<std::size_t>(count);
        }
        return bytes;
    }

    Json receiveMessage(const Socket &socket)
    {
        std::uint32_t size = 0;
        for (const auto byte : receiveExact(socket, 4)) size = (size << 8) | byte;
        check(size > 0 && size <= 65536, "invalid reply size");
        const auto bytes = receiveExact(socket, size);
        return Json::parse(bytes.begin(), bytes.end());
    }

    Json request(std::string type, Json payload, std::string requestId = "request-1")
    {
        return Json{{"version", 2}, {"type", std::move(type)}, {"requestId", std::move(requestId)}, {"payload", std::move(payload)}};
    }

    Json exchangeMessage(const Socket &socket, const Json &message)
    {
        sendBytes(socket, frame(message));
        const auto reply = receiveMessage(socket);
        check(reply.at("version") == 2 && reply.at("requestId") == message.at("requestId"), "reply correlation lost");
        return reply;
    }

    void checkClosed(const Socket &socket)
    {
        char byte;
        const int count = static_cast<int>(::recv(socket.get(), &byte, 1, 0));
        check(count == 0 || (count < 0 && disconnected(socketError())), "connection was not closed");
    }

    void checkRejected(const Json &reply, const char *code)
    {
        check(reply.at("type") == "request.rejected" && reply.at("payload").at("code") == code, "unexpected rejection");
        check(!reply.at("payload").contains("userToken"), "rejection leaked a token");
    }

    Json acceptedResult(const Json &reply)
    {
        check(reply.at("type") == "request.accepted" && reply.at("payload").size() == 1 && reply.at("payload").contains("result"), "missing command result envelope");
        return reply.at("payload").at("result");
    }

    struct TestDirectory
    {
        fs::path path = fs::absolute(fs::current_path()) / ("admission-test-" + UserToken::generate().value());
        TestDirectory() { check(fs::create_directory(path), "cannot create test directory"); }
        ~TestDirectory()
        {
            std::error_code ignored;
            // Fixtures own this one-level directory; never remove recursively.
            for (fs::directory_iterator iterator(path, ignored), end; iterator != end && !ignored; iterator.increment(ignored))
            {
                std::error_code error;
                fs::remove(iterator->path(), error);
            }
            fs::remove(path, ignored);
        }
    };

    std::string readText(const fs::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        check(static_cast<bool>(file), "cannot read archive");
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    struct Fixture
    {
        ResourceManager resources;
        ProtocolManager protocol{resources};
        RoleManager roles{resources, "host"};
        ~Fixture() { try { protocol.stopHosting(); } catch (...) { } }
    };

    Json join(const Socket &socket, const std::string &code)
    {
        const auto reply = exchangeMessage(socket, request("session.join", {{"sessionCode", code}}));
        const auto payload = acceptedResult(reply);
        check(payload.size() == 3 && payload.at("role") == "player", "join did not create a Player");
        check(payload.at("userId").get<std::string>() != payload.at("userToken").get<std::string>(), "public ID exposes the credential");
        static_cast<void>(UserToken(payload.at("userToken").get<std::string>()));
        return payload;
    }

    Json resume(const Socket &socket, const std::string &code, const std::string &token)
    {
        const auto reply = exchangeMessage(socket, request("session.resume", {{"sessionCode", code}, {"userToken", token}}, "resume-1"));
        const auto payload = acceptedResult(reply);
        check(payload.size() == 2 && !payload.contains("userToken"), "resume returned an unexpected token or payload");
        return payload;
    }

    void commonDispatchBeforeAdmission()
    {
        TestDirectory directory;
        Fixture fixture;
        const auto archive = directory.path / "identities.json";
        fixture.protocol.saveIdentityState(archive);
        fixture.resources.invoke<ProtocolManager::AddCommandSignature>("protocol.add_command", "test.public", Role::Any, [](const CommandContext &context, const Json &) { return Json(context.userId.has_value()); });
        std::atomic<int> playerCalls{0};
        fixture.protocol.add_command("test.player", Role::Player, [&](const auto &, const auto &) { return Json(++playerCalls); });
        fixture.protocol.add_command("test.invalid.public", Role::Any, [](const auto &, const auto &) -> Json { throw std::invalid_argument("invalid public payload"); });
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        const auto pending = connectClient(port);
        check(acceptedResult(exchangeMessage(pending, request(" test.public ", Json::object()))) == false, "anonymous Any command was not dispatched");
        const auto saved = Json::parse(readText(archive));
        check(saved.at("users").size() == 1 && saved.at("tokens").empty(), "Any command created an identity");
        checkRejected(exchangeMessage(pending, request("test.player", {{"userId", "host"}, {"role", "host"}})), "FORBIDDEN");
        checkClosed(pending);
        check(playerCalls == 0, "anonymous user reached an authenticated callback");
        const auto invalid = connectClient(port);
        checkRejected(exchangeMessage(invalid, request("test.invalid.public", Json::object())), "INVALID_REQUEST");
        checkClosed(invalid);
        const auto client = connectClient(port);
        const auto joined = acceptedResult(exchangeMessage(client, request(" session.join ", {{"sessionCode", code}}, "padded-join")));
        check(joined.at("role") == "player" && joined.size() == 3, "padded join did not use the registered command");
        check(acceptedResult(exchangeMessage(client, request("test.public", Json::object()))) == true, "newly admitted Any context has no identity");
        check(acceptedResult(exchangeMessage(client, request("test.player", Json::object()))) == 1, "join did not enable the assigned Player role");
        checkRejected(exchangeMessage(client, request(" session.resume ", {{"sessionCode", code}, {"userToken", joined.at("userToken")}})), "ALREADY_ADMITTED");
        const auto replacement = connectClient(port);
        const auto resumed = acceptedResult(exchangeMessage(replacement, request(" session.resume ", {{"sessionCode", code}, {"userToken", joined.at("userToken")}}, "padded-resume")));
        check(resumed.size() == 2 && resumed.at("userId") == joined.at("userId") && !resumed.contains("userToken"), "resume result did not preserve the common command contract");
        checkClosed(client);
        check(acceptedResult(exchangeMessage(replacement, request("test.public", Json::object()))) == true, "resumed Any context lost identity");
        check(fixture.roles.toSaveData().size() == 2, "anonymous command handling duplicated participants");
        fixture.protocol.stopHosting();
    }

    void joinResumeAndReplacement()
    {
        TestDirectory directory;
        Fixture fixture;
        const auto archive = directory.path / "identities.json";
        fixture.protocol.saveIdentityState(archive);
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        auto code = fixture.protocol.getSessionCode();
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char value) { return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value; });
        const auto original = connectClient(port);
        const auto joinBytes = frame(request("session.join", {{"sessionCode", code}}, "fragmented"));
        sendBytes(original, {joinBytes.begin(), joinBytes.begin() + 2});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        sendBytes(original, {joinBytes.begin() + 2, joinBytes.end()});
        const auto reply = receiveMessage(original);
        check(reply.at("requestId") == "fragmented", "fragmented join lost correlation");
        const auto result = acceptedResult(reply);
        const auto userId = result.at("userId").get<std::string>();
        const auto token = result.at("userToken").get<std::string>();
        check(fixture.protocol.resolveUserToken(token) == userId && fixture.roles.getRole(userId) == Role::Player, "join did not connect managers");

        const auto other = connectClient(port);
        const auto otherUser = join(other, code);
        check(otherUser.at("userId") != userId && otherUser.at("userToken") != token, "new clients share an identity");
        check(otherUser.at("userToken") != token, "another client received the original credential");
        fixture.resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", "host", userId, Role::Master);
        const auto wrongCode = connectClient(port);
        checkRejected(exchangeMessage(wrongCode, request("session.resume", {{"sessionCode", "!!!!!"}, {"userToken", token}})), "INVALID_SESSION_CODE");
        checkClosed(wrongCode);
        const auto wrongToken = connectClient(port);
        checkRejected(exchangeMessage(wrongToken, request("session.resume", {{"sessionCode", code}, {"userToken", "bad"}})), "INVALID_USER_TOKEN");
        checkClosed(wrongToken);
        checkRejected(exchangeMessage(original, request("test.echo", Json::object())), "UNKNOWN_COMMAND");
        const auto replacement = connectClient(port);
        const auto resumed = resume(replacement, code, token);
        check(resumed.at("userId") == userId && resumed.at("role") == "master", "resume reset the current role");
        checkClosed(original);
        checkRejected(exchangeMessage(replacement, request("users.setRole", {{"role", "host"}})), "UNKNOWN_COMMAND");
        checkRejected(exchangeMessage(replacement, request("session.join", {{"sessionCode", code}})), "ALREADY_ADMITTED");
        checkRejected(exchangeMessage(replacement, request("session.resume", {{"sessionCode", code}, {"userToken", otherUser.at("userToken")}})), "ALREADY_ADMITTED");
        checkRejected(exchangeMessage(other, request("test.echo", Json::object())), "UNKNOWN_COMMAND");
        check(fixture.roles.toSaveData().size() == 3 && fixture.protocol.resolveUserToken(token) == userId, "resume duplicated or rotated identity");
        fixture.protocol.saveIdentityState(archive);
        const auto saved = readText(archive);
        check(saved.find(token) == std::string::npos && saved.find(otherUser.at("userToken").get<std::string>()) == std::string::npos, "archive contains an original credential");
        check(Json::parse(saved).at("users").at(0).size() == 2, "display name persisted");
        fixture.protocol.stopHosting();
        checkClosed(replacement);
        checkClosed(other);
    }

    void rejectedAdmissionsHaveNoEffects()
    {
        TestDirectory directory;
        Fixture fixture;
        const auto archive = directory.path / "identities.json";
        fixture.protocol.saveIdentityState(archive);
        const auto saved = readText(archive);
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        const std::vector<std::pair<Json, const char *>> requests{
            {request("session.join", Json::object()), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", 1}}), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", code}, {"userId", "host"}}), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", code}, {"role", "host"}}), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", code}, {"displayName", "Alice"}}), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", code}, {"userToken", "0000000000000000"}}), "INVALID_REQUEST"},
            {request("session.join", {{"sessionCode", "!!!!!"}}), "INVALID_SESSION_CODE"},
            {request("session.resume", {{"sessionCode", code}}), "INVALID_REQUEST"},
            {request("session.resume", {{"sessionCode", code}, {"userToken", 1}}), "INVALID_REQUEST"},
            {request("session.resume", {{"sessionCode", code}, {"userToken", "0000000000000000"}}), "INVALID_USER_TOKEN"},
            {request("session.resume", {{"sessionCode", code}, {"userToken", "ABCDEF0123456789"}}), "INVALID_USER_TOKEN"},
            {request("session.resume", {{"sessionCode", code}, {"userToken", "bad"}}), "INVALID_USER_TOKEN"},
            {request("users.setRole", {{"userId", "host"}, {"role", "host"}}), "UNKNOWN_COMMAND"}
        };
        for (const auto &[message, expected] : requests)
        {
            const auto client = connectClient(port);
            checkRejected(exchangeMessage(client, message), expected);
            checkClosed(client);
            check(fixture.roles.toSaveData().size() == 1 && readText(archive) == saved, "rejected admission changed identities");
        }
        Fixture foreign;
        foreign.protocol.saveIdentityState(directory.path / "foreign.json");
        foreign.roles.registerUser("foreign");
        const auto foreignToken = foreign.protocol.issueUserToken("foreign");
        const auto client = connectClient(port);
        checkRejected(exchangeMessage(client, request("session.resume", {{"sessionCode", code}, {"userToken", foreignToken}})), "INVALID_USER_TOKEN");
        checkClosed(client);
        const auto valid = connectClient(port);
        static_cast<void>(join(valid, code));
        fixture.protocol.stopHosting();
    }

    void storageFailureRollsBackAndAllowsRetry()
    {
        TestDirectory directory;
        Fixture fixture;
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        const auto unconfigured = connectClient(port);
        checkRejected(exchangeMessage(unconfigured, request("session.join", {{"sessionCode", code}})), "ADMISSION_FAILED");
        checkClosed(unconfigured);
        check(fixture.roles.toSaveData().size() == 1, "unconfigured admission registered a user");
        const auto archive = directory.path / "identities.json";
        fixture.protocol.saveIdentityState(archive);
        const auto original = connectClient(port);
        const auto user = join(original, code);
        const auto saved = readText(archive);
        const auto backup = directory.path / "backup.json";
        fs::rename(archive, backup);
        fs::create_directory(archive);
        const auto failed = connectClient(port);
        checkRejected(exchangeMessage(failed, request("session.join", {{"sessionCode", code}})), "ADMISSION_FAILED");
        checkClosed(failed);
        check(fixture.roles.toSaveData().size() == 2, "failed save left a half-registered Player");
        check(fixture.protocol.resolveUserToken(user.at("userToken").get<std::string>()) == user.at("userId").get<std::string>(), "failed admission invalidated an existing token");
        check(readText(backup) == saved, "failed admission damaged the prior archive");
        check(std::distance(fs::directory_iterator(directory.path), fs::directory_iterator{}) == 2, "temporary file leaked after failed admission");
        fs::remove(archive);
        fs::rename(backup, archive);
        const auto retry = connectClient(port);
        static_cast<void>(join(retry, code));
        check(fixture.roles.toSaveData().size() == 3, "retry did not register exactly one Player");
        fixture.protocol.stopHosting();
    }

    void restoreAndHostingRestart()
    {
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        std::string token;
        std::string userId;
        {
            Fixture first;
            first.protocol.saveIdentityState(archive);
            const auto client = connectClient(first.protocol.startHosting("127.0.0.1", 0));
            const auto user = join(client, first.protocol.getSessionCode());
            token = user.at("userToken").get<std::string>();
            userId = user.at("userId").get<std::string>();
            first.roles.setRole("host", userId, Role::Master);
            first.protocol.saveIdentityState(archive);
            first.protocol.stopHosting();
            checkClosed(client);
        }
        Fixture restored;
        restored.protocol.loadIdentityState(archive);
        const auto code = restored.protocol.getSessionCode();
        const auto client = connectClient(restored.protocol.startHosting("127.0.0.1", 0));
        check(resume(client, code, token).at("role") == "master", "archive restore lost the assigned role");
        restored.protocol.stopHosting();
        checkClosed(client);
        const auto replacement = connectClient(restored.protocol.startHosting("127.0.0.1", 0));
        check(resume(replacement, code, token).at("userId") == userId, "hosting restart lost identity");
        restored.protocol.stopHosting();

        Session session("host");
        session.loadIdentityState(archive);
        const auto throughSession = connectClient(session.startHosting("127.0.0.1", 0));
        check(resume(throughSession, session.getSessionCode(), token).at("userId") == userId, "Session facade did not admit a restored user");
        session.stopHosting();
    }

    void bufferedFramesAndAdmissionDeadline()
    {
        TestDirectory directory;
        Fixture fixture;
        fixture.protocol.saveIdentityState(directory.path / "identities.json");
        int calls = 0;
        fixture.protocol.add_command("test.echo", Role::Player, [&](const CommandContext &, const Json &) { return Json(++calls); });
        fixture.protocol.add_command("test.public", Role::Any, [](const CommandContext &context, const Json &) { return Json(context.userId.has_value()); });
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto client = connectClient(port);
        const auto pending = connectClient(port);
        check(acceptedResult(exchangeMessage(pending, request("test.public", Json::object()))) == false, "public command admitted the pending peer");
        auto bytes = frame(request("session.join", {{"sessionCode", fixture.protocol.getSessionCode()}}, "join"));
        for (const auto id : {"first", "second"})
        {
            const auto command = frame(request("test.echo", {{"userId", "host"}}, id));
            bytes.insert(bytes.end(), command.begin(), command.end());
        }
        sendBytes(client, bytes);
        const auto joined = receiveMessage(client);
        check(joined.at("requestId") == "join" && acceptedResult(joined).at("role") == "player", "coalesced join failed");
        int expected = 0;
        for (const auto id : {"first", "second"})
        {
            const auto reply = receiveMessage(client);
            check(reply.at("requestId") == id, "buffered request lost or reordered");
            check(acceptedResult(reply) == ++expected, "buffered command did not execute in order");
        }
        check(acceptedResult(exchangeMessage(client, request("test.echo", {{"padding", std::string(64000, 'x')}}, "large-frame"))) == 3, "large command frame failed");
        std::this_thread::sleep_for(std::chrono::milliseconds(5300));
        checkClosed(pending);
        check(acceptedResult(exchangeMessage(client, request("test.echo", Json::object(), "after-timeout"))) == 4, "admitted command stream expired");
        sendBytes(client, {0, 0, 0, 1, '{'});
        checkRejected(receiveMessage(client), "INVALID_REQUEST");
        checkClosed(client);
        check(fixture.roles.toSaveData().size() == 2, "disconnect removed persistent membership");
        fixture.protocol.stopHosting();
        check(calls == 4, "commands were lost or executed twice");
    }

    void bufferedRequestsAreDiscardedDuringStop()
    {
        using namespace std::chrono_literals;
        TestDirectory directory;
        std::promise<void> stagingEntered;
        auto stagingSeen = stagingEntered.get_future();
        std::promise<void> releaseStaging;
        auto stagingReleased = releaseStaging.get_future().share();
        std::promise<void> tailEntered;
        auto tailSeen = tailEntered.get_future();
        std::promise<void> releaseTail;
        auto tailReleased = releaseTail.get_future().share();
        std::atomic<int> noops{0};
        std::atomic<int> tails{0};
        std::atomic<int> completed{0};
        Fixture fixture;
        fixture.protocol.saveIdentityState(directory.path / "identities.json");
        fixture.protocol.add_command("test.stage", Role::Player, [&](const auto &, const auto &)
        {
            stagingEntered.set_value();
            check(stagingReleased.wait_for(3s) == std::future_status::ready, "staging callback was not released");
            return Json(true);
        });
        fixture.protocol.add_command("test.noop", Role::Player, [&](const auto &, const auto &) { return Json(++noops); });
        fixture.protocol.add_command("test.tail", Role::Player, [&](const auto &, const auto &)
        {
            const int count = ++tails;
            if (count == 1)
            {
                tailEntered.set_value();
                check(tailReleased.wait_for(3s) == std::future_status::ready, "buffered callback was not released");
            }
            ++completed;
            return Json(count);
        });
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        const auto gate = connectClient(port);
        const auto first = connectClient(port);
        const auto second = connectClient(port);
        static_cast<void>(join(gate, code));
        const auto user = join(first, code);
        static_cast<void>(join(second, code));
        const auto userId = user.at("userId").get<std::string>();
        const auto token = user.at("userToken").get<std::string>();
        fixture.roles.setRole("host", userId, Role::Master);

        sendBytes(gate, frame(request("test.stage", Json::object())));
        check(stagingSeen.wait_for(2s) == std::future_status::ready, "staging callback did not start");
        auto pipeline = frame(request("test.noop", Json::object(), "noop"));
        const auto tail = frame(request("test.tail", Json::object(), "tail"));
        pipeline.insert(pipeline.end(), tail.begin(), tail.end());
        sendBytes(first, pipeline);
        sendBytes(second, pipeline);
        std::this_thread::sleep_for(50ms);
        releaseStaging.set_value();
        check(acceptedResult(receiveMessage(gate)) == true, "staging command failed");
        check(tailSeen.wait_for(2s) == std::future_status::ready, "buffered callback did not start");
        check(noops == 2 && tails == 1, "both peers did not stage their buffered requests");
        check(receiveMessage(first).at("requestId") == "noop", "first peer lost its preceding response");
        check(receiveMessage(second).at("requestId") == "noop", "second peer lost its preceding response");

        std::promise<void> stopEntered;
        auto stopSeen = stopEntered.get_future();
        auto stopping = std::async(std::launch::async, [&]
        {
            stopEntered.set_value();
            fixture.protocol.stopHosting();
        });
        check(stopSeen.wait_for(1s) == std::future_status::ready, "stop thread did not start");
        const bool waited = stopping.wait_for(100ms) == std::future_status::timeout;
        releaseTail.set_value();
        check(stopping.wait_for(3s) == std::future_status::ready, "stop did not finish after callback completion");
        stopping.get();
        check(waited, "stop did not wait for the active request");
        check(tails == 1, "stop started another peer's buffered request");
        check(completed == 1, "stop did not let the active request finish");
        checkClosed(gate);
        checkClosed(first);
        checkClosed(second);
        check(fixture.roles.toSaveData().size() == 4 && fixture.roles.getRole(userId) == Role::Master, "stop changed persisted identities or roles");
        check(fixture.protocol.resolveUserToken(token) == userId && fixture.protocol.getSessionCode() == code, "stop lost token recognition or session code");

        check(fixture.protocol.startHosting("127.0.0.1", port) == port, "stop retained the listener port");
        const auto returning = connectClient(port);
        check(resume(returning, code, token).at("role") == "master", "restart lost the returning user's role");
        check(acceptedResult(exchangeMessage(returning, request("test.tail", Json::object()))) == 2, "restart replayed discarded work or rejected new work");
        fixture.protocol.stopHosting();
        checkClosed(returning);
    }

    void stopDiscardsRequestsWaitingForExecution()
    {
        using namespace std::chrono_literals;
        TestDirectory directory;
        std::promise<void> localEntered;
        auto localSeen = localEntered.get_future();
        std::promise<void> releaseLocal;
        auto localReleased = releaseLocal.get_future().share();
        std::atomic<int> remoteCalls{0};
        Fixture fixture;
        fixture.protocol.saveIdentityState(directory.path / "identities.json");
        fixture.protocol.add_command("test.local", Role::Host, [&](const auto &, const auto &)
        {
            localEntered.set_value();
            check(localReleased.wait_for(3s) == std::future_status::ready, "local callback was not released");
            return Json(true);
        });
        fixture.protocol.add_command("test.remote", Role::Any, [&](const auto &, const auto &) { return Json(++remoteCalls); });
        const auto client = connectClient(fixture.protocol.startHosting("127.0.0.1", 0));
        static_cast<void>(join(client, fixture.protocol.getSessionCode()));
        auto local = std::async(std::launch::async, [&] { return fixture.protocol.dispatch("test.local", CommandContext{"host"}, Json::object()); });
        check(localSeen.wait_for(2s) == std::future_status::ready, "local callback did not start");
        sendBytes(client, frame(request("test.remote", Json::object())));
        std::this_thread::sleep_for(50ms);
        std::promise<void> stopEntered;
        auto stopSeen = stopEntered.get_future();
        auto stopping = std::async(std::launch::async, [&]
        {
            stopEntered.set_value();
            fixture.protocol.stopHosting();
        });
        check(stopSeen.wait_for(1s) == std::future_status::ready, "stop thread did not start");
        const bool waited = stopping.wait_for(100ms) == std::future_status::timeout;
        releaseLocal.set_value();
        check(local.get() == true && waited, "stop interrupted the active local callback");
        check(stopping.wait_for(3s) == std::future_status::ready, "stop blocked behind a queued remote request");
        stopping.get();
        check(remoteCalls == 0, "remote request ran after waiting for execution during stop");
        checkClosed(client);
        check(fixture.roles.toSaveData().size() == 2, "stop removed admitted identities");
    }

    void authenticatedCommandsAndCurrentPermissions()
    {
        TestDirectory directory;
        Fixture fixture;
        fixture.protocol.saveIdentityState(directory.path / "identities.json");
        fixture.resources.invoke<ProtocolManager::AddCommandSignature>("protocol.add_command", "test.echo", Role::Player, [](const CommandContext &context, const Json &payload)
        {
            return Json{{"actor", *context.userId}, {"connection", *context.connectionId}, {"payload", payload}};
        });
        std::atomic<int> privilegedCalls{0};
        fixture.protocol.add_command("test.master", Role::Master, [&](const auto &, const auto &) { ++privilegedCalls; return Json("ok"); });
        fixture.protocol.add_command("test.value", Role::Spectator, [](const auto &, const Json &payload) { return payload.at("value"); });
        fixture.protocol.add_command("test.nested", Role::Player, [&, registered = false](const CommandContext &context, const Json &payload) mutable
        {
            if (!registered)
            {
                fixture.resources.invoke<ProtocolManager::AddCommandSignature>("protocol.add_command", "test.inner", Role::Player, [count = 0](const auto &, const auto &) mutable { return Json(++count); });
                registered = true;
            }
            return fixture.protocol.dispatch("test.inner", context, payload);
        });
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        const auto client = connectClient(port);
        const auto user = join(client, code);
        const auto userId = user.at("userId").get<std::string>();
        const auto token = user.at("userToken").get<std::string>();
        const Json spoofed{{"userId", "host"}, {"role", "host"}};
        const auto echo = acceptedResult(exchangeMessage(client, request("test.echo", spoofed)));
        check(echo.at("actor") == userId && echo.at("connection").get<ConnectionId>() != 0 && echo.at("payload") == spoofed, "payload overrode the authenticated context");
        checkRejected(exchangeMessage(client, request("test.master", spoofed)), "FORBIDDEN");
        check(privilegedCalls == 0, "unauthorized callback ran");
        fixture.roles.setRole("host", userId, Role::Master);
        check(acceptedResult(exchangeMessage(client, request("test.master", Json::object()))) == "ok", "promotion did not apply to network dispatch");
        fixture.roles.setRole("host", userId, Role::Spectator);
        checkRejected(exchangeMessage(client, request("test.master", spoofed)), "FORBIDDEN");
        checkRejected(exchangeMessage(client, request("test.echo", spoofed)), "FORBIDDEN");
        check(privilegedCalls == 1, "demotion did not prevent callback execution");
        for (const auto &value : std::vector<Json>{nullptr, 7, true, "ciao", Json::array({1, 2}), Json{{"value", 3}}})
        {
            check(acceptedResult(exchangeMessage(client, request("test.value", {{"value", value}}))) == value, "JSON result changed type or value");
        }
        checkRejected(exchangeMessage(client, request("not.registered", Json::object())), "UNKNOWN_COMMAND");
        fixture.roles.setRole("host", userId, Role::Player);
        check(acceptedResult(exchangeMessage(client, request("test.nested", Json::object()))) == 1, "nested network dispatch failed");
        check(acceptedResult(exchangeMessage(client, request("test.nested", Json::object()))) == 2, "callback state was copied or reset");

        fixture.protocol.add_command("test.counter", Role::Spectator, [count = 0](const auto &, const auto &) mutable { return Json(++count); });
        auto local = std::async(std::launch::async, [&]
        {
            for (int index = 0; index < 50; ++index)
            {
                fixture.protocol.add_command("runtime." + std::to_string(index), Role::Player, [](const auto &, const auto &) { return Json(true); });
                static_cast<void>(fixture.protocol.dispatch("test.counter", CommandContext{"host"}, Json::object()));
            }
        });
        for (int index = 0; index < 20; ++index) static_cast<void>(acceptedResult(exchangeMessage(client, request("test.counter", Json::object()))));
        local.get();
        check(acceptedResult(exchangeMessage(client, request("test.counter", Json::object()))) == 71, "local and remote callbacks were not serialized");

        const auto replacement = connectClient(port);
        check(resume(replacement, code, token).at("userId") == userId, "resume lost authenticated identity");
        checkClosed(client);
        check(acceptedResult(exchangeMessage(replacement, request("test.echo", Json::object()))).at("actor") == userId, "replacement command used a stale identity");
        fixture.protocol.stopHosting();
    }

    void callbackFailuresAndInvalidResults()
    {
        TestDirectory directory;
        Fixture fixture;
        fixture.protocol.saveIdentityState(directory.path / "identities.json");
        fixture.protocol.add_command("test.ok", Role::Player, [](const auto &, const auto &) { return Json(true); });
        fixture.protocol.add_command("test.invalid", Role::Player, [](const auto &, const auto &) -> Json { throw std::invalid_argument("private validation detail"); });
        fixture.protocol.add_command("test.payload", Role::Player, [](const auto &, const Json &payload) { return Json(payload.at("required").get<int>()); });
        fixture.protocol.add_command("test.forbidden", Role::Player, [](const auto &, const auto &) -> Json { throw ProtocolError(ProtocolErrorCode::Forbidden, "private authorization detail"); });
        fixture.protocol.add_command("test.failure", Role::Player, [](const auto &, const auto &) -> Json { throw std::runtime_error("private storage detail"); });
        fixture.protocol.add_command("test.nonstandard", Role::Player, [](const auto &, const auto &) -> Json { throw 42; });
        fixture.protocol.add_command("test.lifecycle", Role::Player, [&](const auto &, const auto &) { fixture.protocol.stopHosting(); return Json(true); });
        std::atomic<int> invalidResults{0};
        fixture.protocol.add_command("test.result", Role::Any, [&](const auto &, const Json &payload) -> Json
        {
            ++invalidResults;
            const auto kind = payload.at("kind").get<std::string>();
            if (kind == "large") return Json(std::string(65536, 'x'));
            if (kind == "infinite") return Json(std::numeric_limits<double>::infinity());
            if (kind == "binary") return Json::binary({0, 1});
            if (kind == "utf8") return Json(std::string(1, static_cast<char>(0xff)));
            Json result = nullptr;
            for (int depth = 0; depth < 64; ++depth) result = Json::array({std::move(result)});
            return result;
        });
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto pending = connectClient(port);
        checkRejected(exchangeMessage(pending, request("test.ok", Json::object())), "FORBIDDEN");
        checkClosed(pending);
        const auto invalidAnonymous = connectClient(port);
        checkRejected(exchangeMessage(invalidAnonymous, request("test.result", {{"kind", "large"}})), "COMMAND_FAILED");
        checkClosed(invalidAnonymous);
        const auto client = connectClient(port);
        static_cast<void>(join(client, fixture.protocol.getSessionCode()));
        for (const auto &[name, code] : std::vector<std::pair<const char *, const char *>>{
            {"test.invalid", "INVALID_REQUEST"}, {"test.payload", "INVALID_REQUEST"}, {"test.forbidden", "FORBIDDEN"},
            {"test.failure", "COMMAND_FAILED"}, {"test.nonstandard", "COMMAND_FAILED"}, {"test.lifecycle", "COMMAND_FAILED"}})
        {
            const auto reply = exchangeMessage(client, request(name, Json::object()));
            checkRejected(reply, code);
            check(reply.dump().find("private") == std::string::npos, "internal exception details were sent to client");
            check(acceptedResult(exchangeMessage(client, request("test.ok", Json::object()))) == true, "callback failure closed the admitted connection or stopped hosting");
        }
        checkRejected(exchangeMessage(client, request("test.payload", {{"required", "wrong type"}})), "INVALID_REQUEST");
        for (const auto kind : {"large", "infinite", "binary", "utf8", "deep"})
        {
            checkRejected(exchangeMessage(client, request("test.result", {{"kind", kind}})), "COMMAND_FAILED");
            check(acceptedResult(exchangeMessage(client, request("test.ok", Json::object()))) == true, "invalid result closed the stream");
        }
        check(invalidResults == 6, "failed result serialization retried a callback");
        fixture.protocol.stopHosting();
    }

    void concurrentLocalRolesAndNetworkAdmissions()
    {
        TestDirectory directory;
        Fixture fixture;
        fixture.roles.registerUser("local");
        const auto archive = directory.path / "identities.json";
        fixture.protocol.saveIdentityState(archive);
        const auto port = fixture.protocol.startHosting("127.0.0.1", 0);
        const auto code = fixture.protocol.getSessionCode();
        auto local = std::async(std::launch::async, [&]
        {
            for (int index = 0; index < 40; ++index)
            {
                fixture.roles.registerUser("local-" + std::to_string(index));
                fixture.roles.setRole("host", "local", index % 2 == 0 ? Role::Master : Role::Spectator);
                check(fixture.roles.getRole("host") == Role::Host, "concurrent role mutation changed host");
                static_cast<void>(fixture.roles.toSaveData());
            }
            fixture.roles.setRole("host", "local", Role::Master);
        });
        std::vector<std::future<Json>> clients;
        for (int index = 0; index < 8; ++index)
        {
            clients.push_back(std::async(std::launch::async, [&]
            {
                const auto client = connectClient(port);
                return join(client, code);
            }));
        }
        std::set<std::string> ids;
        for (auto &client : clients) ids.insert(client.get().at("userId").get<std::string>());
        local.get();
        check(ids.size() == 8 && fixture.roles.toSaveData().size() == 50, "concurrent registration lost or duplicated users");
        check(fixture.roles.getRole("local") == Role::Master, "network admission overwrote a local role change");
        fixture.protocol.saveIdentityState(archive);
        fixture.protocol.stopHosting();
        Fixture restored;
        restored.protocol.loadIdentityState(archive);
        check(restored.roles.toSaveData().size() == 50 && restored.roles.getRole("local") == Role::Master, "concurrent state was not persisted");
    }
}

int main()
{
#if defined(_WIN32)
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 1;
#endif
    int result = 0;
    try
    {
        commonDispatchBeforeAdmission();
        joinResumeAndReplacement();
        rejectedAdmissionsHaveNoEffects();
        storageFailureRollsBackAndAllowsRetry();
        restoreAndHostingRestart();
        bufferedFramesAndAdmissionDeadline();
        bufferedRequestsAreDiscardedDuringStop();
        stopDiscardsRequestsWaitingForExecution();
        authenticatedCommandsAndCurrentPermissions();
        callbackFailuresAndInvalidResults();
        concurrentLocalRolesAndNetworkAdmissions();
        std::cout << "admission: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "admission: " << error.what() << '\n';
        result = 1;
    }
#if defined(_WIN32)
    WSACleanup();
#endif
    return result;
}
