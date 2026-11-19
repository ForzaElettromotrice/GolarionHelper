#include "socket_support.hpp"
#include "client_connection.hpp"
#include "protocol.hpp"
#include "permission_json.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    using Clock = std::chrono::steady_clock;
    using Deadline = std::optional<Clock::time_point>;

    [[noreturn]] void invalidReply()
    {
        throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid host reply");
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

    struct CommandReply
    {
        Json result;
        std::exception_ptr failure;
    };

    CommandReply parseCommandReply(const detail::ProtocolMessage &reply)
    {
        try
        {
            const auto &payload = reply.payload;
            if (reply.type == "request.accepted" && payload.size() == 1 && payload.contains("result")) return {payload.at("result"), {}};
            if (reply.type != "request.rejected" || payload.size() != 2 || !payload.at("code").is_string() || !payload.at("message").is_string()) invalidReply();
            const auto code = parseError(payload.at("code").get_ref<const std::string &>());
            return {{}, std::make_exception_ptr(ProtocolError(code, payload.at("message").get<std::string>()))};
        }
        catch (const Json::exception &) { invalidReply(); }
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
            if (result.size() != (resume ? 2u : 3u) || !result.at("userId").is_string()) invalidReply();
            const auto userId = result.at("userId").get<UserId>();
            if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; })) invalidReply();
            ClientAdmission admission{userId, detail::parsePermissions(result.at("permissions")), std::nullopt};
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
        struct Pending
        {
            std::promise<Json> result;
            Clock::time_point deadline;
            bool sent = false;
        };
        struct Outgoing
        {
            std::string requestId;
            std::vector<std::uint8_t> bytes;
            std::size_t sent = 0;
        };

        ProtocolManager &owner;
        const ConnectionId connectionId;
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
        std::unordered_map<std::string, Pending> pending;
        std::deque<Outgoing> outgoing;
        std::thread worker;

        Impl(ProtocolManager &manager, ConnectionId connection, std::string ip, std::uint16_t port, SessionCode sessionCode, std::optional<UserToken> userToken, std::string id) : owner(manager), connectionId(connection), address(detail::ipv4Address(ip, port)), code(std::move(sessionCode)), token(std::move(userToken)), requestId(std::move(id))
        {
            if (port == 0) throw std::invalid_argument("client port must be nonzero");
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

        bool acceptsContext(const CommandContext &context) const
        {
            const std::lock_guard lock(stateMutex);
            return !context.userId && context.connectionId == connectionId && state.state == ClientConnectionState::Connected && !stopRequested;
        }

        void updatePermissions(PermissionSet permissions)
        {
            const std::lock_guard lock(stateMutex);
            state.permissions = permissions;
        }

        void updateParticipants(ParticipantsSnapshot participants)
        {
            const std::lock_guard lock(stateMutex);
            if (state.participants)
            {
                if (participants.revision < state.participants->revision) return;
                if (participants.revision == state.participants->revision)
                {
                    if (participants != *state.participants) invalidReply();
                    return;
                }
                const auto host = [](const auto &snapshot) { return std::find_if(snapshot.participants.begin(), snapshot.participants.end(), [](const auto &participant) { return participant.isHost; })->userId; };
                if (host(participants) != host(*state.participants)) invalidReply();
            }
            if (!state.userId || std::none_of(participants.participants.begin(), participants.participants.end(), [this](const auto &participant) { return participant.userId == *state.userId; })) invalidReply();
            state.participants = std::move(participants);
        }

        std::future<Json> sendCommand(std::string name, Json payload, std::string id)
        {
            auto bytes = detail::encodeMessage({std::move(name), id, std::move(payload)});
            Pending request{{}, Clock::now() + std::chrono::seconds(5)};
            auto result = request.result.get_future();
            const std::lock_guard lock(stateMutex);
            if (state.state != ClientConnectionState::Connected || stopRequested) throw std::logic_error("client must be admitted before sending commands");
            if (pending.size() >= 64) throw std::length_error("too many outstanding client commands");
            const auto [entry, inserted] = pending.emplace(id, std::move(request));
            if (!inserted) throw std::logic_error("duplicate client request ID");
            try { outgoing.push_back({std::move(id), std::move(bytes)}); }
            catch (...) { pending.erase(entry); throw; }
            return result;
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

        void processMessages()
        {
            while (!input.empty())
            {
                checkStop();
                std::optional<detail::ProtocolMessage> reply;
                try { reply = detail::decodeMessage(input); }
                catch (const std::invalid_argument &) { invalidReply(); }
                if (!reply) return;
                if (!reply->requestId)
                {
                    if (reply->type == "request.accepted" || reply->type == "request.rejected") invalidReply();
                    // Dispatch without state/lifecycle locks; callbacks may read the snapshot.
                    static_cast<void>(owner.dispatch(reply->type, CommandContext{std::nullopt, connectionId}, reply->payload));
                    continue;
                }
                auto parsed = parseCommandReply(*reply);
                const std::lock_guard lock(stateMutex);
                const auto entry = pending.find(*reply->requestId);
                if (entry == pending.end() || !entry->second.sent) invalidReply();
                if (Clock::now() >= entry->second.deadline) throw std::system_error(std::make_error_code(std::errc::timed_out), "client command timed out");
                if (parsed.failure) entry->second.result.set_exception(parsed.failure);
                else entry->second.result.set_value(std::move(parsed.result));
                pending.erase(entry);
            }
        }

        void monitor()
        {
            std::optional<Outgoing> current;
            while (true)
            {
                checkStop();
                processMessages();
                long waitMicroseconds = 50000;
                {
                    const std::lock_guard lock(stateMutex);
                    const auto now = Clock::now();
                    for (const auto &[id, request] : pending)
                    {
                        static_cast<void>(id);
                        if (now >= request.deadline) throw std::system_error(std::make_error_code(std::errc::timed_out), "client command timed out");
                        waitMicroseconds = static_cast<long>(std::min<std::int64_t>(waitMicroseconds, std::max<std::int64_t>(1, std::chrono::duration_cast<std::chrono::microseconds>(request.deadline - now).count())));
                    }
                    if (!current && !outgoing.empty())
                    {
                        current.emplace(std::move(outgoing.front()));
                        outgoing.pop_front();
                    }
                }
                fd_set readable;
                fd_set writable;
                FD_ZERO(&readable);
                FD_ZERO(&writable);
                FD_SET(socket.get(), &readable);
                if (current) FD_SET(socket.get(), &writable);
                timeval timeout{0, waitMicroseconds};
#if defined(_WIN32)
                const int selected = ::select(0, &readable, &writable, nullptr, &timeout);
#else
                const int selected = ::select(socket.get() + 1, &readable, &writable, nullptr, &timeout);
#endif
                if (selected < 0)
                {
                    if (detail::retryable(detail::socketError())) continue;
                    detail::fail("select");
                }
                checkStop();
                if (current && FD_ISSET(socket.get(), &writable))
                {
                    int flags = 0;
#if defined(MSG_NOSIGNAL)
                    flags = MSG_NOSIGNAL;
#endif
                    const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(current->bytes.data() + current->sent), static_cast<int>(current->bytes.size() - current->sent), flags));
                    if (count < 0)
                    {
                        if (!detail::retryable(detail::socketError())) detail::fail("send");
                    }
                    else
                    {
                        if (count == 0) throw std::system_error(std::make_error_code(std::errc::connection_reset), "host closed during command send");
                        current->sent += static_cast<std::size_t>(count);
                        if (current->sent == current->bytes.size())
                        {
                            const std::lock_guard lock(stateMutex);
                            pending.at(current->requestId).sent = true;
                            current.reset();
                        }
                    }
                }
                if (FD_ISSET(socket.get(), &readable))
                {
                    std::array<std::uint8_t, 4096> bytes{};
                    const auto capacity = detail::MaximumFrameBytes + 4 - input.size();
                    const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(std::min(bytes.size(), capacity)), 0));
                    if (count < 0)
                    {
                        if (!detail::retryable(detail::socketError())) detail::fail("recv");
                    }
                    else
                    {
                        if (count == 0) throw std::system_error(std::make_error_code(std::errc::connection_reset), "host disconnected");
                        input.insert(input.end(), bytes.begin(), bytes.begin() + count);
                    }
                }
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
                ClientSnapshot connected{ClientConnectionState::Connected, result.userId, result.permissions};
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
                outgoing.clear();
                for (auto &[id, request] : pending)
                {
                    static_cast<void>(id);
                    request.result.set_exception(failure);
                }
                pending.clear();
            }
            if (!completed)
            {
                try { admission.set_exception(failure); }
                catch (...) { } // A worker must not propagate a promise-state failure.
            }
        }
    };

    ProtocolManager::ClientConnection::ClientConnection(ProtocolManager &owner, ConnectionId connectionId, std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::string requestId) : impl_(std::make_unique<Impl>(owner, connectionId, std::move(ip), port, std::move(code), std::move(token), std::move(requestId))) {}
    ProtocolManager::ClientConnection::~ClientConnection() = default;
    std::future<ClientAdmission> ProtocolManager::ClientConnection::takeResult() { return impl_->admission.get_future(); }
    void ProtocolManager::ClientConnection::start() { impl_->worker = std::thread([this] { impl_->run(); }); }
    ClientSnapshot ProtocolManager::ClientConnection::snapshot() const { return impl_->snapshot(); }
    bool ProtocolManager::ClientConnection::acceptsContext(const CommandContext &context) const { return impl_->acceptsContext(context); }
    void ProtocolManager::ClientConnection::updatePermissions(PermissionSet permissions) { impl_->updatePermissions(permissions); }
    void ProtocolManager::ClientConnection::updateParticipants(ParticipantsSnapshot participants) { impl_->updateParticipants(std::move(participants)); }
    std::future<nlohmann::json> ProtocolManager::ClientConnection::sendCommand(std::string name, nlohmann::json payload, std::string requestId) { return impl_->sendCommand(std::move(name), std::move(payload), std::move(requestId)); }
    void ProtocolManager::ClientConnection::stop() { impl_->stop(); }
}
