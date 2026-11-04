#if defined(_WIN32) && !defined(_WIN32_WINNT)
    #define _WIN32_WINNT 0x0601
#endif

#include "golarion/play/network.hpp"
#include "protocol.hpp"

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <netdb.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
#if defined(_WIN32)
    using NativeSocket = SOCKET;
    constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
    int socketError() { return WSAGetLastError(); }
    bool interrupted(int error) { return error == WSAEINTR; }
    void closeSocket(NativeSocket socket) { closesocket(socket); }
#else
    using NativeSocket = int;
    constexpr NativeSocket InvalidSocket = -1;
    int socketError() { return errno; }
    bool interrupted(int error) { return error == EINTR; }
    void closeSocket(NativeSocket socket) { ::close(socket); }
#endif

    [[noreturn]] void fail(std::string_view operation)
    {
        throw std::system_error(socketError(), std::system_category(), std::string(operation));
    }

    class SocketRuntime final
    {
    public:
        SocketRuntime()
        {
#if defined(_WIN32)
            WSADATA data{};
            const int error = WSAStartup(MAKEWORD(2, 2), &data);
            if (error != 0) throw std::system_error(error, std::system_category(), "WSAStartup");
#endif
        }
        ~SocketRuntime()
        {
#if defined(_WIN32)
            WSACleanup();
#endif
        }
        SocketRuntime(const SocketRuntime &) = delete;
        SocketRuntime &operator=(const SocketRuntime &) = delete;
    };

    class Socket final
    {
    public:
        explicit Socket(NativeSocket handle = InvalidSocket) : handle_(handle) {}
        ~Socket() { if (handle_ != InvalidSocket) closeSocket(handle_); }
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket(Socket &&other) noexcept : handle_(std::exchange(other.handle_, InvalidSocket)) {}
        Socket &operator=(Socket &&other) noexcept
        {
            if (this != &other)
            {
                if (handle_ != InvalidSocket) closeSocket(handle_);
                handle_ = std::exchange(other.handle_, InvalidSocket);
            }
            return *this;
        }
        NativeSocket get() const { return handle_; }

    private:
        NativeSocket handle_;
    };

    void configureSocket(NativeSocket socket)
    {
#if defined(_WIN32)
        const DWORD timeout = 1000;
#else
        const timeval timeout{1, 0};
#endif
        if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) != 0) fail("setsockopt");
#if defined(SO_NOSIGPIPE)
        const int enabled = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) fail("setsockopt");
#endif
#if !defined(_WIN32)
        if (socket >= FD_SETSIZE) throw std::runtime_error("socket exceeds select descriptor capacity");
#endif
    }

    bool sendBytes(NativeSocket socket, const std::vector<std::uint8_t> &bytes)
    {
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int sent = static_cast<int>(::send(socket, reinterpret_cast<const char *>(bytes.data() + offset), static_cast<int>(bytes.size() - offset), flags));
            if (sent < 0 && interrupted(socketError())) continue;
            if (sent <= 0) return false;
            offset += static_cast<std::size_t>(sent);
        }
        return true;
    }

    bool receiveBytes(NativeSocket socket, std::vector<std::uint8_t> &buffer)
    {
        std::array<std::uint8_t, 4096> bytes{};
        const int received = static_cast<int>(::recv(socket, reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), 0));
        if (received < 0 && interrupted(socketError())) return true;
        if (received <= 0) return false;
        buffer.insert(buffer.end(), bytes.begin(), bytes.begin() + received);
        if (buffer.size() > 2 * (golarion::play::detail::MaximumFrameBytes + 4)) throw std::invalid_argument("receive buffer exceeded");
        return true;
    }

    int waitForRead(fd_set &readSet, NativeSocket maximum, std::chrono::milliseconds timeout)
    {
        if (timeout.count() < 0 || timeout > std::chrono::seconds(60)) throw std::invalid_argument("poll timeout must be between 0 and 60 seconds");
        timeval duration{};
        duration.tv_sec = static_cast<long>(timeout.count() / 1000);
        duration.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
#if defined(_WIN32)
        static_cast<void>(maximum);
        const int result = ::select(0, &readSet, nullptr, nullptr, &duration);
#else
        const int result = ::select(maximum + 1, &readSet, nullptr, nullptr, &duration);
#endif
        if (result < 0 && interrupted(socketError())) return 0;
        if (result < 0) fail("select");
        return result;
    }

    Socket connectSocket(std::string_view host, std::uint16_t port)
    {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo *rawAddresses = nullptr;
        const std::string hostname(host);
        const std::string service = std::to_string(port);
        if (getaddrinfo(hostname.c_str(), service.c_str(), &hints, &rawAddresses) != 0) throw std::runtime_error("cannot resolve host address");
        const std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addresses(rawAddresses, freeaddrinfo);
        int lastError = 0;
        for (const addrinfo *address = addresses.get(); address; address = address->ai_next)
        {
            Socket socket(::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
            if (socket.get() == InvalidSocket)
            {
                lastError = socketError();
                continue;
            }
            if (::connect(socket.get(), address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0)
            {
                // On POSIX, SO_SNDTIMEO also limits connect(), so configure it only after the connection succeeds.
                configureSocket(socket.get());
                return socket;
            }
            lastError = socketError();
        }
        throw std::system_error(lastError, std::system_category(), "connect");
    }
}

namespace golarion::play
{
    struct GameHost::Impl
    {
        struct Connection
        {
            Socket socket;
            std::vector<std::uint8_t> input;
            std::optional<std::string> participantId;
            std::chrono::steady_clock::time_point acceptedAt = std::chrono::steady_clock::now();
        };

        SocketRuntime runtime;
        Game game;
        Socket listener;
        std::uint16_t port = 0;
        std::map<NativeSocket, Connection> connections;

        Impl(Game initialGame, std::string_view address, std::uint16_t requestedPort) : game(std::move(initialGame)), listener(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
        {
            if (!game.snapshot().connectedParticipants.empty()) throw std::invalid_argument("a new host must start without live participants");
            if (listener.get() == InvalidSocket) fail("socket");
            configureSocket(listener.get());
            sockaddr_in endpoint{};
            endpoint.sin_family = AF_INET;
            endpoint.sin_port = htons(requestedPort);
            const std::string addressText(address);
            if (inet_pton(AF_INET, addressText.c_str(), &endpoint.sin_addr) != 1) throw std::invalid_argument("host bind address must be an IPv4 address");
            if (::bind(listener.get(), reinterpret_cast<const sockaddr *>(&endpoint), sizeof(endpoint)) != 0) fail("bind");
            if (::listen(listener.get(), SOMAXCONN) != 0) fail("listen");
#if defined(_WIN32)
            int size = sizeof(endpoint);
#else
            socklen_t size = sizeof(endpoint);
#endif
            if (getsockname(listener.get(), reinterpret_cast<sockaddr *>(&endpoint), &size) != 0) fail("getsockname");
            port = ntohs(endpoint.sin_port);
        }

        void remove(NativeSocket handle)
        {
            const auto connection = connections.find(handle);
            if (connection == connections.end()) return;
            if (connection->second.participantId) game.disconnectParticipant(*connection->second.participantId);
            connections.erase(connection);
        }

        void broadcast()
        {
            bool removed = false;
            do
            {
                removed = false;
                const auto bytes = detail::encodeSnapshot(game.snapshot());
                std::vector<NativeSocket> failed;
                for (const auto &[handle, connection] : connections)
                {
                    if (connection.participantId && !sendBytes(handle, bytes)) failed.push_back(handle);
                }
                for (NativeSocket handle : failed)
                {
                    remove(handle);
                    removed = true;
                }
            } while (removed);
        }

        void poll(std::chrono::milliseconds timeout)
        {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(listener.get(), &readSet);
            NativeSocket maximum = listener.get();
            for (const auto &[handle, connection] : connections)
            {
                static_cast<void>(connection);
                FD_SET(handle, &readSet);
                maximum = std::max(maximum, handle);
            }
            const int ready = waitForRead(readSet, maximum, timeout);
            if (ready > 0 && FD_ISSET(listener.get(), &readSet))
            {
                Socket socket(::accept(listener.get(), nullptr, nullptr));
                if (socket.get() != InvalidSocket && connections.size() < std::min<std::size_t>(32, FD_SETSIZE - 1))
                {
                    configureSocket(socket.get());
                    const NativeSocket handle = socket.get();
                    connections.emplace(handle, Connection{std::move(socket), {}, std::nullopt});
                }
            }

            bool changed = false;
            std::vector<NativeSocket> closed;
            for (auto &[handle, connection] : connections)
            {
                if (!connection.participantId && std::chrono::steady_clock::now() - connection.acceptedAt > std::chrono::seconds(5))
                {
                    closed.push_back(handle);
                    continue;
                }
                if (ready <= 0 || !FD_ISSET(handle, &readSet)) continue;
                try
                {
                    if (!receiveBytes(handle, connection.input))
                    {
                        closed.push_back(handle);
                        continue;
                    }
                    while (auto message = detail::extractMessage(connection.input))
                    {
                        const auto *join = std::get_if<detail::JoinRequest>(&*message);
                        if (!join || connection.participantId) throw std::invalid_argument("expected a single join request");
                        const Participant participant = game.connectParticipant(join->name);
                        connection.participantId = participant.id;
                        changed = true;
                    }
                }
                catch (const std::invalid_argument &)
                {
                    sendBytes(handle, detail::encodeRejection("Richiesta di collegamento non valida."));
                    closed.push_back(handle);
                }
            }
            for (NativeSocket handle : closed)
            {
                changed = changed || connections.at(handle).participantId.has_value();
                remove(handle);
            }
            if (changed) broadcast();
        }
    };

    GameHost::GameHost(Game game, std::string_view bindAddress, std::uint16_t port) : impl_(std::make_unique<Impl>(std::move(game), bindAddress, port)) {}
    GameHost::~GameHost() = default;
    std::uint16_t GameHost::port() const { return impl_->port; }
    void GameHost::poll(std::chrono::milliseconds timeout) { impl_->poll(timeout); }

    struct GameClient::Impl
    {
        SocketRuntime runtime;
        Socket socket;
        std::vector<std::uint8_t> input;

        Impl(std::string_view host, std::uint16_t port, std::string_view name) : socket(connectSocket(host, port))
        {
            if (!sendBytes(socket.get(), detail::encodeJoin(name))) throw std::runtime_error("cannot send join request");
        }

        std::optional<GameSnapshot> poll(std::chrono::milliseconds timeout)
        {
            if (auto message = detail::extractMessage(input)) return snapshot(std::move(*message));
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(socket.get(), &readSet);
            if (waitForRead(readSet, socket.get(), timeout) == 0) return std::nullopt;
            if (!receiveBytes(socket.get(), input)) throw std::runtime_error("host connection closed");
            if (auto message = detail::extractMessage(input)) return snapshot(std::move(*message));
            return std::nullopt;
        }

        static GameSnapshot snapshot(detail::Message message)
        {
            if (auto *snapshot = std::get_if<GameSnapshot>(&message)) return std::move(*snapshot);
            if (const auto *rejected = std::get_if<detail::ConnectionRejected>(&message)) throw std::runtime_error(rejected->reason);
            throw std::runtime_error("unexpected server message");
        }
    };

    GameClient::GameClient(std::string_view host, std::uint16_t port, std::string_view participantName) : impl_(std::make_unique<Impl>(host, port, participantName)) {}
    GameClient::~GameClient() = default;
    std::optional<GameSnapshot> GameClient::poll(std::chrono::milliseconds timeout) { return impl_->poll(timeout); }
}
