#pragma once

#if defined(_WIN32) && !defined(_WIN32_WINNT)
    #define _WIN32_WINNT 0x0601
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace golarion::play::detail
{
#if defined(_WIN32)
    using NativeSocket = SOCKET;
    using AddressLength = int;
    inline constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
    inline int socketError() { return WSAGetLastError(); }
    inline bool retryable(int error) { return error == WSAEINTR || error == WSAEWOULDBLOCK; }
    inline bool connectInProgress(int error) { return retryable(error) || error == WSAEINPROGRESS || error == WSAEALREADY; }
    inline void closeSocket(NativeSocket socket) { closesocket(socket); }
#else
    using NativeSocket = int;
    using AddressLength = socklen_t;
    inline constexpr NativeSocket InvalidSocket = -1;
    inline int socketError() { return errno; }
    inline bool retryable(int error) { return error == EINTR || error == EAGAIN || error == EWOULDBLOCK; }
    inline bool connectInProgress(int error) { return error == EINPROGRESS || error == EALREADY || error == EINTR; }
    inline void closeSocket(NativeSocket socket) { ::close(socket); }
#endif

    [[noreturn]] inline void fail(std::string_view operation)
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
        NativeSocket get() const noexcept { return handle_; }

    private:
        NativeSocket handle_;
    };

    inline sockaddr_in ipv4Address(std::string_view ip, std::uint16_t port)
    {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        if (ip.find('\0') != std::string_view::npos || inet_pton(AF_INET, std::string(ip).c_str(), &address.sin_addr) != 1)
        {
            throw std::invalid_argument("address must be a numeric IPv4 address");
        }
        return address;
    }

    inline void configureSocket(NativeSocket socket)
    {
#if defined(_WIN32)
        u_long enabled = 1;
        if (ioctlsocket(socket, FIONBIO, &enabled) != 0) fail("ioctlsocket");
#else
        if (socket >= FD_SETSIZE) throw std::runtime_error("socket exceeds select descriptor capacity");
        const int flags = fcntl(socket, F_GETFL, 0);
        if (flags < 0 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0) fail("fcntl");
#endif
#if defined(SO_NOSIGPIPE)
        const int noSignal = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const char *>(&noSignal), sizeof(noSignal)) != 0) fail("setsockopt");
#endif
    }
}
