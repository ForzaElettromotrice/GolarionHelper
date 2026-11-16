#include "socket_support.hpp"
#include "client_connection.hpp"
#include "protocol.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    using Clock = std::chrono::steady_clock;
    using Deadline = std::optional<Clock::time_point>;

    [[noreturn]] void invalidReply()
    {
        throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid host admission reply");
    }

    Role parseRole(std::string_view value)
    {
        if (value == "host") return Role::Host;
        if (value == "master") return Role::Master;
        if (value == "player") return Role::Player;
        if (value == "spectator") return Role::Spectator;
        invalidReply();
    }

    ProtocolErrorCode parseError(std::string_view value)
    {
        if (value == "INVALID_REQUEST") return ProtocolErrorCode::InvalidRequest;
        if (value == "INVALID_SESSION_CODE") return ProtocolErrorCode::InvalidSessionCode;
        if (value == "INVALID_USER_TOKEN") return ProtocolErrorCode::InvalidUserToken;
        if (value == "NOT_ADMITTED") return ProtocolErrorCode::NotAdmitted;
        if (value == "ALREADY_ADMITTED") return ProtocolErrorCode::AlreadyAdmitted;
        if (value == "ADMISSION_FAILED") return ProtocolErrorCode::AdmissionFailed;
        if (value == "UNKNOWN_COMMAND") return ProtocolErrorCode::UnknownCommand;
        if (value == "FORBIDDEN") return ProtocolErrorCode::Forbidden;
        if (value == "COMMAND_FAILED") return ProtocolErrorCode::CommandFailed;
        invalidReply();
    }

    ClientAdmission parseAdmission(const detail::ProtocolMessage &reply, const std::string &requestId, bool resume)
    {
        try
        {
            if (reply.requestId != requestId) invalidReply();
            const auto &payload = reply.payload;
            if (reply.type == "request.rejected")
            {
                if (payload.size() != 2 || !payload.at("code").is_string() || !payload.at("message").is_string()) invalidReply();
                throw ProtocolError(parseError(payload.at("code").get_ref<const std::string &>()), payload.at("message").get<std::string>());
            }
            if (reply.type != "request.accepted" || payload.size() != 1 || !payload.at("result").is_object()) invalidReply();
            const auto &result = payload.at("result");
            if (result.size() != (resume ? 2u : 3u) || !result.at("userId").is_string() || !result.at("role").is_string()) invalidReply();
            const auto userId = result.at("userId").get<UserId>();
            if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; })) invalidReply();
            ClientAdmission admission{userId, parseRole(result.at("role").get_ref<const std::string &>()), std::nullopt};
            if (!resume)
            {
                if (!result.at("userToken").is_string()) invalidReply();
                admission.userToken.emplace(result.at("userToken").get_ref<const std::string &>());
            }
            return admission;
        }
        catch (const ProtocolError &) { throw; }
        catch (const Json::exception &) { invalidReply(); }
        catch (const std::invalid_argument &) { invalidReply(); }
    }
}

namespace golarion::play
{
    struct ProtocolManager::ClientConnection::Impl
    {
        detail::SocketRuntime runtime;
        const sockaddr_in address;
        const SessionCode code;
        const std::optional<UserToken> token;
        const std::string requestId;
        detail::Socket socket;
        std::vector<std::uint8_t> input;
        std::atomic<bool> stopRequested{false};
        mutable std::mutex stateMutex;
        ClientSnapshot state{ClientConnectionState::Connecting, std::nullopt, std::nullopt};
        std::promise<ClientAdmission> admission;
        std::thread worker;

        Impl(std::string ip, std::uint16_t port, SessionCode sessionCode, std::optional<UserToken> userToken, std::string id) : address(detail::ipv4Address(ip, port)), code(std::move(sessionCode)), token(std::move(userToken)), requestId(std::move(id))
        {
            if (port == 0) throw std::invalid_argument("client port must be nonzero");
            worker = std::thread([this] { run(); });
        }

        ~Impl() { stop(); }

        void stop()
        {
            stopRequested = true;
            if (worker.joinable()) worker.join();
        }

        void checkStop() const
        {
            if (stopRequested) throw std::system_error(std::make_error_code(std::errc::operation_canceled), "client disconnected");
        }

        ClientSnapshot snapshot() const
        {
            const std::lock_guard lock(stateMutex);
            return state;
        }

        void waitReady(bool writing, Deadline deadline)
        {
            while (true)
            {
                checkStop();
                long waitMicroseconds = 50000;
                if (deadline)
                {
                    const auto remaining = *deadline - Clock::now();
                    if (remaining <= Clock::duration::zero()) throw std::system_error(std::make_error_code(std::errc::timed_out), "client operation timed out");
                    waitMicroseconds = static_cast<long>(std::min<std::int64_t>(waitMicroseconds, std::max<std::int64_t>(1, std::chrono::duration_cast<std::chrono::microseconds>(remaining).count())));
                }
                fd_set ready;
                fd_set failed;
                FD_ZERO(&ready);
                FD_ZERO(&failed);
                FD_SET(socket.get(), &ready);
                FD_SET(socket.get(), &failed);
                timeval timeout{0, waitMicroseconds};
#if defined(_WIN32)
                const int selected = ::select(0, writing ? nullptr : &ready, writing ? &ready : nullptr, &failed, &timeout);
#else
                const int selected = ::select(socket.get() + 1, writing ? nullptr : &ready, writing ? &ready : nullptr, &failed, &timeout);
#endif
                if (selected < 0)
                {
                    if (detail::retryable(detail::socketError())) continue;
                    detail::fail("select");
                }
                checkStop();
                if (selected > 0) return;
            }
        }

        void connect()
        {
            checkStop();
            socket = detail::Socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
            if (socket.get() == detail::InvalidSocket) detail::fail("socket");
            detail::configureSocket(socket.get());
            const auto deadline = Clock::now() + std::chrono::seconds(5);
            if (::connect(socket.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0)
            {
                const int error = detail::socketError();
                if (!detail::connectInProgress(error)) throw std::system_error(error, std::system_category(), "connect");
                waitReady(true, deadline);
                int connectionError = 0;
                detail::AddressLength size = sizeof(connectionError);
                if (getsockopt(socket.get(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&connectionError), &size) != 0) detail::fail("getsockopt");
                if (connectionError != 0) throw std::system_error(connectionError, std::system_category(), "connect");
            }
            checkStop();
            const std::lock_guard lock(stateMutex);
            state.state = ClientConnectionState::Admitting;
        }

        void sendRequest(Clock::time_point deadline)
        {
            Json payload{{"sessionCode", std::string(code.value())}};
            if (token) payload["userToken"] = token->value();
            const auto bytes = detail::encodeMessage({token ? "session.resume" : "session.join", requestId, std::move(payload)});
            std::size_t sent = 0;
            while (sent < bytes.size())
            {
                waitReady(true, deadline);
                int flags = 0;
#if defined(MSG_NOSIGNAL)
                flags = MSG_NOSIGNAL;
#endif
                const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + sent), static_cast<int>(bytes.size() - sent), flags));
                if (count < 0)
                {
                    if (detail::retryable(detail::socketError())) continue;
                    detail::fail("send");
                }
                if (count == 0) throw std::system_error(std::make_error_code(std::errc::connection_reset), "host closed during admission");
                sent += static_cast<std::size_t>(count);
            }
        }

        ClientAdmission receiveReply(Clock::time_point deadline)
        {
            std::array<std::uint8_t, 4096> bytes{};
            while (true)
            {
                std::optional<detail::ProtocolMessage> reply;
                try { reply = detail::decodeMessage(input); }
                catch (const std::invalid_argument &) { invalidReply(); }
                if (reply)
                {
                    return parseAdmission(*reply, requestId, token.has_value());
                }
                waitReady(false, deadline);
                const auto capacity = detail::MaximumFrameBytes + 4 - input.size();
                const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(std::min(bytes.size(), capacity)), 0));
                if (count < 0)
                {
                    if (detail::retryable(detail::socketError())) continue;
                    detail::fail("recv");
                }
                if (count == 0) throw std::system_error(std::make_error_code(std::errc::connection_reset), "host closed before admission reply");
                input.insert(input.end(), bytes.begin(), bytes.begin() + count);
            }
        }

        void monitor()
        {
            // Buffered following bytes belong to the next message, not the admission result.
            if (!input.empty()) invalidReply();
            while (true)
            {
                waitReady(false, std::nullopt);
                std::array<char, 4096> bytes{};
                const int count = static_cast<int>(::recv(socket.get(), bytes.data(), static_cast<int>(bytes.size()), 0));
                if (count == 0) return;
                if (count < 0)
                {
                    if (detail::retryable(detail::socketError())) continue;
                    detail::fail("recv");
                }
                // The first client slice has no outstanding commands or supported host events.
                invalidReply();
            }
        }

        void run() noexcept
        {
            bool completed = false;
            std::exception_ptr failure;
            try
            {
                connect();
                const auto deadline = Clock::now() + std::chrono::seconds(5);
                sendRequest(deadline);
                auto result = receiveReply(deadline);
                ClientSnapshot connected{ClientConnectionState::Connected, result.userId, result.role};
                {
                    const std::lock_guard lock(stateMutex);
                    checkStop();
                    state = std::move(connected);
                    admission.set_value(std::move(result));
                    completed = true;
                }
                monitor();
            }
            catch (...) { failure = std::current_exception(); }
            socket = detail::Socket{};
            {
                const std::lock_guard lock(stateMutex);
                state = ClientSnapshot{};
            }
            if (!completed)
            {
                try { admission.set_exception(failure); }
                catch (...) { } // A worker must not propagate a promise-state failure.
            }
        }
    };

    ProtocolManager::ClientConnection::ClientConnection(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::string requestId) : impl_(std::make_unique<Impl>(std::move(ip), port, std::move(code), std::move(token), std::move(requestId))) {}
    ProtocolManager::ClientConnection::~ClientConnection() = default;
    std::future<ClientAdmission> ProtocolManager::ClientConnection::takeResult() { return impl_->admission.get_future(); }
    ClientSnapshot ProtocolManager::ClientConnection::snapshot() const { return impl_->snapshot(); }
    void ProtocolManager::ClientConnection::stop() { impl_->stop(); }
}
