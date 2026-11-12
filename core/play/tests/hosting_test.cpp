#if defined(_WIN32) && !defined(_WIN32_WINNT)
    #define _WIN32_WINNT 0x0601
#endif

#include "golarion/play/session.hpp"

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <optional>
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
    using Clock = std::chrono::steady_clock;

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
        const int error = socketError();
        throw std::system_error(error, std::system_category(), std::string(operation) + " (" + std::to_string(error) + ")");
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
        if (socket.get() == InvalidSocket) fail("test socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(socket.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0) fail("test connect");
#if defined(_WIN32)
        const DWORD timeout = 1500;
#else
        const timeval timeout{1, 500000};
#endif
        for (const int option : {SO_RCVTIMEO, SO_SNDTIMEO})
        {
            if (setsockopt(socket.get(), SOL_SOCKET, option, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) != 0) fail("test timeout");
        }
        return socket;
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
            if (count <= 0) fail("test send");
            offset += static_cast<std::size_t>(count);
        }
    }

    std::vector<std::uint8_t> frame(const Json &message)
    {
        const auto document = message.dump();
        const auto length = static_cast<std::uint32_t>(document.size());
        std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(length >> 24), static_cast<std::uint8_t>(length >> 16), static_cast<std::uint8_t>(length >> 8), static_cast<std::uint8_t>(length)};
        bytes.insert(bytes.end(), document.begin(), document.end());
        return bytes;
    }

    void receiveExact(const Socket &socket, std::vector<std::uint8_t> &bytes)
    {
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data() + offset), static_cast<int>(bytes.size() - offset), 0));
            check(count > 0, "missing response bytes");
            offset += static_cast<std::size_t>(count);
        }
    }

    Json receiveMessage(const Socket &socket)
    {
        std::vector<std::uint8_t> prefix(4);
        receiveExact(socket, prefix);
        std::uint32_t length = 0;
        for (const auto byte : prefix) length = (length << 8) | byte;
        check(length > 0 && length <= 65536, "invalid response length");
        std::vector<std::uint8_t> document(length);
        receiveExact(socket, document);
        return Json::parse(document.begin(), document.end());
    }

    void checkClosed(const Socket &socket)
    {
        char byte;
        const int received = static_cast<int>(::recv(socket.get(), &byte, 1, 0));
        check(received == 0 || (received < 0 && disconnected(socketError())), "peer was not disconnected");
    }

    Json request(std::string requestId)
    {
        return Json{{"version", 2}, {"type", "users.setRole"}, {"requestId", std::move(requestId)}, {"payload", {{"userId", "host"}, {"role", "host"}}}};
    }

    void pendingConnectionsAndSessionShutdown()
    {
        Session session("host");
        const auto code = session.getSessionCode();
        session.stopHosting();
        check(session.getSessionCode() == code, "inactive stop changed session code");
        const auto port = session.startHosting("127.0.0.1", 0);
        check(session.getSessionCode() == code, "hosting start changed session code");
        check(port != 0, "missing assigned port");
        check(rejects<std::logic_error>([&] { session.startHosting("127.0.0.1", 0); }), "duplicate hosting accepted");
        const auto idle = connectClient(port);
        const auto partial = connectClient(port);
        sendBytes(partial, {0, 0, 1, 0, '{'});

        const auto client = connectClient(port);
        const auto bytes = frame(request("request-1"));
        sendBytes(client, {bytes.begin(), bytes.begin() + 2});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        sendBytes(client, {bytes.begin() + 2, bytes.end()});
        const auto reply = receiveMessage(client);
        check(reply.at("version") == 2 && reply.at("type") == "request.rejected", "unexpected pending reply");
        check(reply.at("requestId") == "request-1", "request correlation lost");
        check(reply.at("payload").at("code") == "UNKNOWN_COMMAND", "unexpected unknown pending command outcome");
        checkClosed(client);

        const auto logical = session.registerConnection();
        session.bindUser(logical, "alice");
        const auto start = Clock::now();
        session.stopHosting();
        check(Clock::now() - start < std::chrono::seconds(2), "shutdown blocked on a peer");
        check(session.getSessionCode() == code, "hosting stop changed session code");
        checkClosed(idle);
        checkClosed(partial);
        check(!session.closeConnection(logical), "shutdown retained a logical connection");
        check(rejects<std::invalid_argument>([&] { session.makeContext(logical); }), "shutdown retained an active binding");
        session.stopHosting();
        check(session.startHosting("127.0.0.1", port) == port, "restart on the same port failed");
        check(session.getSessionCode() == code, "hosting restart changed session code");
    }

    void failedStartupAndPortRelease()
    {
        Session session("host");
        const auto code = session.getSessionCode();
        const auto logical = session.registerConnection();
        session.bindUser(logical, "alice");
        for (const auto address : {"", "localhost", "999.1.2.3", "127.0.0.1 trailing"})
        {
            check(rejects<std::invalid_argument>([&] { session.startHosting(address, 0); }), "invalid address accepted");
        }
        check(session.makeContext(logical).userId == "alice", "failed startup changed bindings");
        check(session.getSessionCode() == code, "failed startup changed session code");
        const auto port = session.startHosting("127.0.0.1", 0);
        Session other("other-host");
        check(rejects<std::system_error>([&] { other.startHosting("127.0.0.1", port); }), "occupied port accepted");
        check(other.startHosting("127.0.0.1", 0) != 0, "failed startup prevented retry");
        session.stopHosting();
        check(session.startHosting("127.0.0.1", port) == port, "listener port was not released");

        std::uint16_t destroyedPort;
        {
            Session temporary("host");
            destroyedPort = temporary.startHosting("127.0.0.1", 0);
        }
        Session replacement("host");
        check(replacement.startHosting("127.0.0.1", destroyedPort) == destroyedPort, "destruction retained listener");
    }

    void invalidMessagesAndNoRemoteDispatch()
    {
        ResourceManager resources;
        ProtocolManager protocol(resources);
        RoleManager roles(resources, "host");
        int calls = 0;
        protocol.add_command("users.setRole", Role::Spectator, [&calls](const CommandContext &, const Json &) { return Json(++calls); });
        const auto port = protocol.startHosting("127.0.0.1", 0);
        for (const auto &bytes : std::array<std::vector<std::uint8_t>, 5>{
            std::vector<std::uint8_t>{0, 0, 0, 1, '{'}, std::vector<std::uint8_t>{0, 1, 0, 1},
            frame(request(std::string(129, 'x'))), frame(Json{{"version", 2}, {"type", "test"}, {"payload", Json::object()}}),
            frame(Json{{"version", 1}, {"type", "test"}, {"requestId", "old"}, {"payload", Json::object()}})})
        {
            const auto client = connectClient(port);
            sendBytes(client, bytes);
            const auto reply = receiveMessage(client);
            check(reply.at("payload").at("code") == "INVALID_REQUEST", "invalid input was not rejected");
            check(!reply.contains("requestId"), "untrusted request ID echoed");
            checkClosed(client);
        }
        const auto client = connectClient(port);
        sendBytes(client, frame(request("valid")));
        check(receiveMessage(client).at("payload").at("code") == "FORBIDDEN", "host did not enforce the minimum role after bad peers");
        protocol.stopHosting();
        check(calls == 0, "unauthenticated callback executed");
    }

    void synchronizedStateAndLifecycle()
    {
        ResourceManager resources;
        ProtocolManager protocol(resources);
        RoleManager roles(resources, "host");
        roles.registerUser("alice");
        roles.setRole("host", "alice", Role::Master);
        const auto start = [&]
        {
            try { return protocol.startHosting("127.0.0.1", 0); }
            catch (const std::logic_error &) { return std::uint16_t{0}; }
        };
        auto firstStart = std::async(std::launch::async, start);
        auto secondStart = std::async(std::launch::async, start);
        const auto firstPort = firstStart.get();
        const auto secondPort = secondStart.get();
        check((firstPort == 0) != (secondPort == 0), "concurrent starts were not serialized");
        const auto port = firstPort != 0 ? firstPort : secondPort;
        std::vector<std::future<void>> tasks;
        for (int index = 0; index < 4; ++index)
        {
            tasks.push_back(std::async(std::launch::async, [&protocol, index]
            {
                for (int iteration = 0; iteration < 100; ++iteration)
                {
                    const auto id = protocol.registerConnection();
                    const auto user = "user-" + std::to_string(index) + "-" + std::to_string(iteration);
                    protocol.bindUser(id, user);
                    check(protocol.getUserId(id) == user, "concurrent lookup lost binding");
                    check(protocol.makeContext(id).connectionId == id, "concurrent context lost connection");
                    check(protocol.closeConnection(id), "concurrent close lost connection");
                }
            }));
        }
        const auto client = connectClient(port);
        sendBytes(client, frame(request("concurrent")));
        check(receiveMessage(client).at("requestId") == "concurrent", "background handling blocked");
        for (auto &task : tasks) task.get();
        const auto oldId = protocol.registerConnection();
        protocol.bindUser(oldId, "alice");
        auto firstStop = std::async(std::launch::async, [&] { protocol.stopHosting(); });
        auto secondStop = std::async(std::launch::async, [&] { protocol.stopHosting(); });
        firstStop.get();
        secondStop.get();
        check(!protocol.getUserId(oldId), "stop retained user presence");
        check(roles.getRole("alice") == Role::Master && roles.getRole("host") == Role::Host, "stop changed roles");
        protocol.startHosting("127.0.0.1", 0);
        check(protocol.registerConnection() != oldId, "restart reused a logical ID");
    }

    void destructionDisconnectsPeers()
    {
        std::optional<Socket> client;
        {
            Session session("host");
            client.emplace(connectClient(session.startHosting("127.0.0.1", 0)));
        }
        checkClosed(*client);
        ResourceManager resources;
        {
            ProtocolManager protocol(resources);
            client.emplace(connectClient(protocol.startHosting("127.0.0.1", 0)));
        }
        checkClosed(*client);
        resources.registerFeature<void()>("protocol.add_command", [] {});
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
        pendingConnectionsAndSessionShutdown();
        failedStartupAndPortRelease();
        invalidMessagesAndNoRemoteDispatch();
        synchronizedStateAndLifecycle();
        destructionDisconnectsPeers();
        std::cout << "hosting: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "hosting: " << error.what() << '\n';
        result = 1;
    }
#if defined(_WIN32)
    WSACleanup();
#endif
    return result;
}
