#include "socket_support.hpp"
#include "hosting.hpp"
#include "admission.hpp"
#include "protocol.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    using golarion::play::detail::ProtocolMessage;
    using golarion::play::detail::AddressLength;
    using golarion::play::detail::InvalidSocket;
    using golarion::play::detail::NativeSocket;
    using golarion::play::detail::Socket;
    using golarion::play::detail::SocketRuntime;
    using golarion::play::detail::configureSocket;
    using golarion::play::detail::fail;
    using golarion::play::detail::retryable;
    using golarion::play::detail::socketError;
}

namespace golarion::play
{
    struct ProtocolManager::Hosting::Impl
    {
        struct Connection
        {
            Socket socket;
            ConnectionId id;
            std::vector<std::uint8_t> input;
            std::vector<std::uint8_t> output;
            std::size_t sent = 0;
            bool closeAfterReply = false;
            Clock::time_point acceptedAt = Clock::now();
        };

        ProtocolManager &owner;
        SocketRuntime runtime;
        Socket listener;
        std::uint16_t boundPort = 0;
        std::map<NativeSocket, Connection> connections;
        std::atomic<bool> stopRequested{false};
        std::exception_ptr workerFailure;
        std::thread worker;

        Impl(ProtocolManager &ownerValue, std::string_view bindAddress, std::uint16_t port) : owner(ownerValue)
        {
            auto address = detail::ipv4Address(bindAddress, port);
            listener = Socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
            if (listener.get() == InvalidSocket) fail("socket");
            configureSocket(listener.get());
            const int enabled = 1;
#if defined(_WIN32)
            constexpr int reuseOption = SO_EXCLUSIVEADDRUSE;
#else
            constexpr int reuseOption = SO_REUSEADDR;
#endif
            if (setsockopt(listener.get(), SOL_SOCKET, reuseOption, reinterpret_cast<const char *>(&enabled), sizeof(enabled)) != 0) fail("setsockopt");
            if (::bind(listener.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0) fail("bind");
            if (::listen(listener.get(), SOMAXCONN) != 0) fail("listen");
            AddressLength addressLength = sizeof(address);
            if (getsockname(listener.get(), reinterpret_cast<sockaddr *>(&address), &addressLength) != 0) fail("getsockname");
            boundPort = ntohs(address.sin_port);
            worker = std::thread([this] { run(); });
        }

        ~Impl() { stop(); }

        void stop()
        {
            stopRequested = true;
            if (worker.joinable()) worker.join();
        }

        void acceptConnection()
        {
            Socket socket(::accept(listener.get(), nullptr, nullptr));
            if (socket.get() == InvalidSocket)
            {
                if (retryable(socketError())) return;
                fail("accept");
            }
            if (connections.size() >= 32 || connections.size() + 1 >= FD_SETSIZE) return;
            try { configureSocket(socket.get()); }
            catch (const std::exception &) { return; }
            const NativeSocket handle = socket.get();
            const ConnectionId id = owner.registerConnection();
            try { connections.emplace(handle, Connection{std::move(socket), id, {}, {}}); }
            catch (...)
            {
                owner.closeConnection(id);
                throw;
            }
        }

        bool currentAndWithinDeadline(const Connection &connection)
        {
            const std::lock_guard lock(owner.connectionsMutex_);
            const auto logical = owner.connections_.find(connection.id);
            return logical != owner.connections_.end() && (logical->second || Clock::now() - connection.acceptedAt < std::chrono::seconds(5));
        }

        void processInput(Connection &connection)
        {
            if (stopRequested) return;
            std::optional<ProtocolMessage> message;
            try
            {
                message = detail::decodeMessage(connection.input);
            }
            catch (const std::invalid_argument &)
            {
                connection.output = detail::encodeMessage(owner.makeRejection(std::nullopt, ProtocolErrorCode::InvalidRequest));
                connection.closeAfterReply = true;
                return;
            }
            if (!message) return;
            MessageResult result;
            {
                const std::lock_guard executionLock(owner.executionMutex_);
                // Stop may have been requested while another local/remote request held execution.
                if (stopRequested) return;
                result = owner.handleMessage(connection.id, *message);
            }
            connection.closeAfterReply = result.closeAfterReply;
            try { connection.output = detail::encodeMessage(result.reply); }
            catch (const std::invalid_argument &)
            {
                // A callback result that cannot be sent is a server failure, not malformed input.
                connection.output = detail::encodeMessage(owner.makeRejection(message->requestId, ProtocolErrorCode::CommandFailed));
                connection.closeAfterReply = !owner.getUserId(connection.id).has_value();
            }
        }

        bool receive(Connection &connection)
        {
            std::array<std::uint8_t, 4096> bytes{};
            const auto capacity = detail::MaximumFrameBytes + 4 - connection.input.size();
            const int count = static_cast<int>(::recv(connection.socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(std::min(bytes.size(), capacity)), 0));
            if (count < 0) return retryable(socketError());
            if (count == 0) return false;
            connection.input.insert(connection.input.end(), bytes.begin(), bytes.begin() + count);
            processInput(connection);
            return true;
        }

        bool send(Connection &connection)
        {
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(connection.socket.get(), reinterpret_cast<const char *>(connection.output.data() + connection.sent), static_cast<int>(connection.output.size() - connection.sent), flags));
            if (count < 0) return retryable(socketError());
            if (count == 0) return false;
            connection.sent += static_cast<std::size_t>(count);
            if (connection.sent < connection.output.size()) return true;
            connection.output.clear();
            connection.sent = 0;
            return !connection.closeAfterReply;
        }

        void poll()
        {
            fd_set readable;
            fd_set writable;
            FD_ZERO(&readable);
            FD_ZERO(&writable);
            FD_SET(listener.get(), &readable);
            NativeSocket maximum = listener.get();
            for (auto &[handle, connection] : connections)
            {
                if (stopRequested) return;
                // Process one buffered frame per peer; keep one bounded reply outstanding.
                if (connection.output.empty() && !connection.input.empty() && currentAndWithinDeadline(connection)) processInput(connection);
                FD_SET(handle, connection.output.empty() ? &readable : &writable);
                maximum = std::max(maximum, handle);
            }
            if (stopRequested) return;
            // Nonblocking sockets and a bounded wait keep shutdown independent of peers.
            timeval timeout{0, 50000};
#if defined(_WIN32)
            const int selected = ::select(0, &readable, &writable, nullptr, &timeout);
#else
            const int selected = ::select(maximum + 1, &readable, &writable, nullptr, &timeout);
#endif
            if (selected < 0)
            {
                if (retryable(socketError())) return;
                fail("select");
            }
            if (stopRequested) return;
            if (FD_ISSET(listener.get(), &readable)) acceptConnection();
            for (auto iterator = connections.begin(); iterator != connections.end() && !stopRequested;)
            {
                auto &[handle, connection] = *iterator;
                bool keep = currentAndWithinDeadline(connection);
                if (keep && FD_ISSET(handle, &readable)) keep = receive(connection);
                if (keep && FD_ISSET(handle, &writable)) keep = send(connection);
                if (keep) ++iterator;
                else
                {
                    owner.closeConnection(connection.id);
                    iterator = connections.erase(iterator);
                }
            }
        }

        void run() noexcept
        {
            try { while (!stopRequested) poll(); }
            catch (...) { workerFailure = std::current_exception(); }
            try
            {
                for (const auto &[handle, connection] : connections) owner.closeConnection(connection.id);
            }
            catch (...) { if (!workerFailure) workerFailure = std::current_exception(); }
            connections.clear();
            listener = Socket{};
        }
    };

    ProtocolManager::Hosting::Hosting(ProtocolManager &owner, std::string_view bindAddress, std::uint16_t port) : impl_(std::make_unique<Impl>(owner, bindAddress, port)) {}
    ProtocolManager::Hosting::~Hosting() = default;
    std::uint16_t ProtocolManager::Hosting::port() const { return impl_->boundPort; }
    void ProtocolManager::Hosting::stop() { impl_->stop(); }
    std::exception_ptr ProtocolManager::Hosting::failure() const { return impl_->workerFailure; }
}
