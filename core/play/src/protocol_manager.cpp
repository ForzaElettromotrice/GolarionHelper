#include "golarion/play/protocol_manager.hpp"
#include "hosting.hpp"
#include "client_connection.hpp"
#include "local_commands.hpp"
#include "identity_storage.hpp"
#include "identity_transaction.hpp"
#include "permission_json.hpp"
#include "participants_json.hpp"
#include "protocol.hpp"
#include "user_tokens.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view AddCommandFeature = "protocol.add_command";
    constexpr std::string_view SendEventFeature = "protocol.sendEvent";
    constexpr std::size_t MaximumQueuedEvents = 64;
    constexpr std::size_t MaximumQueuedEventBytes = 256 * 1024;

    golarion::play::SessionMode validatedMode(golarion::play::SessionMode mode)
    {
        switch (mode)
        {
            case golarion::play::SessionMode::Host:
            case golarion::play::SessionMode::Client: return mode;
        }
        throw std::invalid_argument("unknown Session mode");
    }

    class DispatchScope final
    {
    public:
        explicit DispatchScope(const golarion::play::ProtocolManager &owner) : owner_(owner), previous_(current_) { current_ = this; }
        ~DispatchScope() { current_ = previous_; }
        DispatchScope(const DispatchScope &) = delete;
        DispatchScope &operator=(const DispatchScope &) = delete;

        static bool contains(const golarion::play::ProtocolManager &owner) noexcept
        {
            for (auto scope = current_; scope; scope = scope->previous_)
            {
                if (&scope->owner_ == &owner) return true;
            }
            return false;
        }

    private:
        const golarion::play::ProtocolManager &owner_;
        const DispatchScope *previous_;
        static thread_local const DispatchScope *current_;
    };

    thread_local const DispatchScope *DispatchScope::current_ = nullptr;

    void requireOutsideDispatch(const golarion::play::ProtocolManager &owner)
    {
        // Lifecycle operations may join the worker; never wait for it from a callback.
        if (DispatchScope::contains(owner)) throw std::logic_error("connection lifecycle and identity restoration are not allowed from a command callback");
    }

    void validateUserId(const golarion::play::UserId &userId)
    {
        if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; }))
        {
            throw std::invalid_argument("user ID must not be empty or contain ASCII whitespace/control characters");
        }
    }

    std::string normalizedName(std::string_view name)
    {
        constexpr std::string_view whitespace = " \t\r\n\f\v";
        const auto begin = name.find_first_not_of(whitespace);
        if (begin == std::string_view::npos)
        {
            throw std::invalid_argument("command name must not be empty");
        }
        const auto end = name.find_last_not_of(whitespace);
        return std::string(name.substr(begin, end - begin + 1));
    }

    void validateRequirements(const golarion::play::CommandRequirements &requirements)
    {
        using golarion::play::CommandAccess;
        switch (requirements.access)
        {
            case CommandAccess::Authenticated: return;
            case CommandAccess::Public:
                if (requirements.permissions != golarion::play::PermissionSet{}) throw std::invalid_argument("public commands cannot require participant permissions");
                return;
        }
        throw std::invalid_argument("unknown command access mode");
    }
}

namespace golarion::play
{
    ProtocolManager::ProtocolManager(ResourceManager &resources, SessionMode mode, std::optional<SessionCode> hostCode)
        : resources_(resources), mode_(validatedMode(mode)),
          sessionCode_(mode_ == SessionMode::Host ? (hostCode ? hostCode : std::make_optional(SessionCode::generate())) : std::nullopt),
          userTokens_(mode_ == SessionMode::Host ? std::make_unique<detail::UserTokens>() : nullptr)
    {
        if (mode_ == SessionMode::Client)
        {
            registerClientCommands();
            resources_.registerFeature<AddClientCommandSignature>(AddCommandFeature, [this](std::string_view name, CommandCallback callback) { add_command(name, std::move(callback)); });
            return;
        }
        registerAdmissionCommands();
        registerParticipantCommands();
        resources_.registerFeature<AddCommandSignature>(AddCommandFeature, [this](std::string_view name, CommandRequirements requirements, CommandCallback callback)
        {
            add_command(name, requirements, std::move(callback));
        });
        bool eventFeatureRegistered = false;
        try
        {
            resources_.registerFeature<SendEventSignature>(SendEventFeature, [this](const UserId &userId, std::string_view name, const nlohmann::json &payload) { return sendEvent(userId, name, payload); });
            eventFeatureRegistered = true;
            resources_.registerFeature<detail::UpdateUserPermissionsSignature>(detail::UpdateUserPermissionsFeature, [this](detail::UserPermissionsMutation mutation) { return updateUserPermissions(mutation); });
        }
        catch (...)
        {
            if (eventFeatureRegistered) resources_.unregisterFeature(SendEventFeature);
            resources_.unregisterFeature(AddCommandFeature);
            throw;
        }
    }

    ProtocolManager::~ProtocolManager()
    {
        shutdown();
        try { resources_.unregisterFeature(AddCommandFeature); }
        catch (...) { } // Destruction must not throw if registration was already removed.
        try { if (mode_ == SessionMode::Host) resources_.unregisterFeature(SendEventFeature); }
        catch (...) { }
        try { if (mode_ == SessionMode::Host) resources_.unregisterFeature(detail::UpdateUserPermissionsFeature); }
        catch (...) { }
    }

    void ProtocolManager::shutdown() noexcept
    {
        try
        {
            if (mode_ == SessionMode::Host) stopHosting();
            else disconnectFromHost();
        }
        catch (...) { } // Still join the local worker if networking reported an error.
        try
        {
            const std::lock_guard lock(localCommandsMutex_);
            if (localCommands_) localCommands_->stop();
            localCommands_.reset();
        }
        catch (...) { } // Destruction must not propagate worker errors.
    }

    void ProtocolManager::requireHostMode() const
    {
        if (mode_ != SessionMode::Host) throw std::logic_error("operation requires a host Session");
    }

    void ProtocolManager::requireClientMode() const
    {
        if (mode_ != SessionMode::Client) throw std::logic_error("operation requires a client Session");
    }

    std::future<nlohmann::json> ProtocolManager::sendCommand(std::string name, nlohmann::json payload)
    {
        // A callback must keep its existing actor and use nested dispatch instead.
        // Queuing and waiting here would also deadlock the local worker.
        if (DispatchScope::contains(*this)) throw std::logic_error("sendCommand is not allowed from a command callback");
        name = normalizedName(name);
        if (!payload.is_object()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "command payload must be a JSON object");
        if (mode_ == SessionMode::Host)
        {
            const std::lock_guard lock(localCommandsMutex_);
            if (!localCommands_) localCommands_ = std::make_unique<LocalCommands>(*this);
            return localCommands_->submit(std::move(name), std::move(payload));
        }
        const std::lock_guard lock(clientMutex_);
        if (!client_) throw std::logic_error("client is not connected");
        if (nextClientRequestId_ == 0) throw std::overflow_error("client request ID space is exhausted");
        return client_->sendCommand(std::move(name), std::move(payload), "client-" + std::to_string(nextClientRequestId_++));
    }

    void ProtocolManager::add_command(std::string_view name, CommandRequirements requirements, CommandCallback callback)
    {
        requireHostMode();
        registerCommand(name, requirements, std::move(callback));
    }

    void ProtocolManager::add_command(std::string_view name, CommandCallback callback)
    {
        requireClientMode();
        registerCommand(name, Permission::Any, std::move(callback));
    }

    void ProtocolManager::registerCommand(std::string_view name, CommandRequirements requirements, CommandCallback callback)
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = normalizedName(name);
        validateRequirements(requirements);
        if (mode_ == SessionMode::Client && (key == "request.accepted" || key == "request.rejected")) throw std::invalid_argument("reply names are reserved");
        if (!callback)
        {
            throw std::invalid_argument("command callback must not be empty: " + key);
        }
        if (commands_.contains(key))
        {
            throw std::invalid_argument("command is already registered: " + key);
        }
        commands_.emplace(key, Command{requirements, std::move(callback)});
    }

    nlohmann::json ProtocolManager::dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = normalizedName(name);
        if (!payload.is_object())
        {
            throw ProtocolError(ProtocolErrorCode::InvalidRequest, "command payload must be a JSON object");
        }
        if (mode_ == SessionMode::Client)
        {
            const auto connection = activeClient_.lock();
            if (!connection || !connection->acceptsContext(context)) throw ProtocolError(ProtocolErrorCode::NotAdmitted, "notification must come from the currently admitted host connection");
        }
        else if ((!context.userId && !context.connectionId) || (context.userId && context.userId->empty()))
        {
            throw ProtocolError(ProtocolErrorCode::NotAdmitted, "command context must identify a live connection or a recognized local user");
        }
        if (mode_ == SessionMode::Host && context.connectionId && makeContext(*context.connectionId).userId != context.userId)
        {
            throw ProtocolError(ProtocolErrorCode::NotAdmitted, "command context no longer matches the active connection");
        }
        const auto entry = commands_.find(key);
        if (entry == commands_.end())
        {
            throw ProtocolError(ProtocolErrorCode::UnknownCommand, "command is not registered: " + key);
        }
        const Command &command = entry->second;
        bool authorized = mode_ == SessionMode::Client || command.requirements.access == CommandAccess::Public;
        if (mode_ == SessionMode::Host && context.userId)
        {
            try { authorized = resources_.invoke<PermissionManager::HasPermissionsSignature>("permissions.hasPermissions", *context.userId, command.requirements.permissions); }
            catch (...)
            {
                throw ProtocolError(ProtocolErrorCode::CommandFailed, "cannot verify the command permission requirement");
            }
        }
        if (!authorized)
        {
            throw ProtocolError(ProtocolErrorCode::Forbidden, "user does not have the required permissions for command: " + key);
        }
        const DispatchScope scope(*this);
        return command.callback(context, payload);
    }

    void ProtocolManager::registerClientCommands()
    {
        add_command("session.participantsChanged", [this](const CommandContext &, const nlohmann::json &payload)
        {
            try { activeClient_.lock()->updateParticipants(detail::parseParticipants(payload)); }
            catch (const nlohmann::json::exception &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid participant notification"); }
            catch (const std::invalid_argument &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid participant notification"); }
            return nlohmann::json(nullptr);
        });
        add_command("permissions.changed", [this](const CommandContext &, const nlohmann::json &payload)
        {
            try
            {
                const auto connection = activeClient_.lock(); // dispatch already holds executionMutex_.
                const auto state = connection->snapshot();
                if (payload.size() != 2 || !payload.at("userId").is_string() || payload.at("userId") != *state.userId) throw std::invalid_argument("permission notification targets another user");
                connection->updatePermissions(detail::parsePermissions(payload.at("permissions")));
            }
            catch (const nlohmann::json::exception &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission notification"); }
            catch (const std::invalid_argument &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission notification"); }
            return nlohmann::json(nullptr);
        });
    }

    bool ProtocolManager::sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload)
    {
        requireHostMode();
        validateUserId(userId);
        auto key = normalizedName(name);
        if (key == "request.accepted" || key == "request.rejected") throw std::invalid_argument("reply names cannot be sent as notifications");
        auto frame = detail::encodeMessage({std::move(key), std::nullopt, payload});
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard connectionsLock(connectionsMutex_);
        const auto active = activeConnections_.find(userId);
        if (active == activeConnections_.end()) return false;
        const std::lock_guard eventsLock(eventsMutex_);
        if (!acceptingEvents_) return false;
        auto &queue = events_[active->second];
        if (queue.overflowed) return false;
        if (queue.frames.size() >= MaximumQueuedEvents || frame.size() > MaximumQueuedEventBytes - queue.bytes)
        {
            // Disconnect a slow peer rather than silently leaving its state stale.
            queue.overflowed = true;
            queue.frames.clear();
            queue.bytes = 0;
            return false;
        }
        const auto size = frame.size();
        queue.frames.push_back(std::move(frame));
        queue.bytes += size;
        return true;
    }

    std::optional<std::vector<std::uint8_t>> ProtocolManager::takeEvent(ConnectionId connectionId)
    {
        const std::lock_guard lock(eventsMutex_);
        const auto entry = events_.find(connectionId);
        if (!acceptingEvents_ || entry == events_.end() || entry->second.overflowed || entry->second.frames.empty()) return std::nullopt;
        auto &queue = entry->second;
        auto frame = std::move(queue.frames.front());
        queue.frames.pop_front();
        queue.bytes -= frame.size();
        return frame;
    }

    bool ProtocolManager::eventQueueOverflowed(ConnectionId connectionId) const
    {
        const std::lock_guard lock(eventsMutex_);
        const auto entry = events_.find(connectionId);
        return entry != events_.end() && entry->second.overflowed;
    }

    std::string ProtocolManager::getSessionCode() const
    {
        return std::string(hostCode().value());
    }

    const SessionCode &ProtocolManager::hostCode() const
    {
        requireHostMode();
        return *sessionCode_;
    }

    bool ProtocolManager::verifySessionCode(std::string_view candidate) const noexcept
    {
        return sessionCode_ && sessionCode_->matches(candidate);
    }

    std::string ProtocolManager::issueUserToken(const UserId &userId)
    {
        requireHostMode();
        const std::lock_guard lock(identityMutex_);
        static_cast<void>(resources_.invoke<PermissionManager::GetPermissionsSignature>("permissions.getPermissions", userId));
        if (!identityArchivePath_) throw std::logic_error("save or load an identity archive before issuing tokens");
        auto candidate = std::make_unique<detail::UserTokens>(*userTokens_);
        auto token = candidate->issue(userId);
        const IdentitySaveData data{resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId"), resources_.invoke<PermissionManager::ToSaveDataSignature>("permissions.toSaveData"), candidate->toSaveData()};
        detail::writeIdentityState(*identityArchivePath_, data);
        userTokens_.swap(candidate);
        return token;
    }

    std::optional<UserId> ProtocolManager::resolveUserToken(std::string_view token) const
    {
        requireHostMode();
        const std::lock_guard lock(identityMutex_);
        return userTokens_->resolve(token);
    }

    PermissionSet ProtocolManager::updateUserPermissions(const std::function<PermissionSet(const PermissionManager::PermissionsCommit &)> &mutation)
    {
        requireHostMode();
        if (!mutation) throw std::invalid_argument("permission mutation must not be empty");
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard identityLock(identityMutex_);
        if (!identityArchivePath_) throw ProtocolError(ProtocolErrorCode::CommandFailed, "configure an identity archive before updating permissions");
        const auto hostId = resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId");
        const auto tokens = userTokens_->toSaveData();
        bool committed = false;
        const PermissionManager::PermissionsCommit commit = [&](const std::vector<UserPermissionsSaveData> &users)
        {
            if (committed) throw std::logic_error("a permission mutation must commit once");
            try { detail::writeIdentityState(*identityArchivePath_, IdentitySaveData{hostId, users, tokens}); }
            catch (...) { throw ProtocolError(ProtocolErrorCode::CommandFailed, "cannot persist the permission change"); }
            committed = true;
        };
        const auto result = mutation(commit);
        if (!committed) throw std::logic_error("permission mutation did not commit");
        return result;
    }

    ProtocolManager::HostingSnapshot ProtocolManager::getHostingSnapshot() const
    {
        requireHostMode();
        requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        const auto port = hosting_ ? std::make_optional(hosting_->port()) : std::nullopt;
        return {port, getParticipants()};
    }

    IdentitySaveData ProtocolManager::identityDataLocked() const
    {
        return {
            resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId"),
            resources_.invoke<PermissionManager::ToSaveDataSignature>("permissions.toSaveData"),
            userTokens_->toSaveData()
        };
    }

    IdentitySaveData ProtocolManager::toIdentityData() const
    {
        requireHostMode();
        const std::lock_guard lock(identityMutex_);
        return identityDataLocked();
    }

    void ProtocolManager::saveIdentityState(const std::filesystem::path &path)
    {
        requireHostMode();
        if (path.empty()) throw std::invalid_argument("identity archive path must not be empty");
        std::optional<std::filesystem::path> destination(std::filesystem::absolute(path).lexically_normal());
        const std::lock_guard lock(identityMutex_);
        if (std::filesystem::exists(*destination) && (!identityArchivePath_ || *identityArchivePath_ != *destination))
        {
            throw std::logic_error("load an existing identity archive before replacing it");
        }
        detail::writeIdentityState(*destination, identityDataLocked());
        identityArchivePath_.swap(destination);
    }

    void ProtocolManager::loadIdentityState(const std::filesystem::path &path)
    {
        requireHostMode();
        requireOutsideDispatch(*this);
        if (path.empty()) throw std::invalid_argument("identity archive path must not be empty");
        std::optional<std::filesystem::path> source(std::filesystem::absolute(path).lexically_normal());
        const std::lock_guard hostingLock(hostingMutex_);
        if (hosting_) throw std::logic_error("stop hosting before restoring identities");
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard identityLock(identityMutex_);
        const auto data = detail::readIdentityState(*source);
        if (data.hostId != resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId")) throw std::invalid_argument("saved host does not match this Session's host");
        auto candidate = std::make_unique<detail::UserTokens>(detail::UserTokens::fromSaveData(data.tokens));
        bool changed;
        {
            const std::lock_guard connectionsLock(connectionsMutex_);
            resources_.invoke<PermissionManager::RestoreSaveDataSignature>("permissions.restoreSaveData", data.users);
            userTokens_.swap(candidate);
            identityArchivePath_.swap(source);
            changed = !activeConnections_.empty();
            connections_.clear();
            activeConnections_.clear();
        }
        if (changed) publishParticipantsChanged();
    }

    ConnectionId ProtocolManager::registerConnection()
    {
        requireHostMode();
        const std::lock_guard lock(connectionsMutex_);
        if (nextConnectionId_ == 0)
        {
            throw std::overflow_error("connection ID space is exhausted");
        }
        const ConnectionId connectionId = nextConnectionId_;
        connections_.emplace(connectionId, std::nullopt);
        // Zero is reserved; unsigned wraparound permanently exhausts this manager's IDs.
        ++nextConnectionId_;
        return connectionId;
    }

    std::optional<ConnectionId> ProtocolManager::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        requireHostMode();
        validateUserId(userId);
        const std::lock_guard executionLock(executionMutex_);
        std::unique_lock lock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end())
        {
            throw std::invalid_argument("connection is not registered");
        }
        if (connection->second)
        {
            if (*connection->second != userId)
            {
                throw std::invalid_argument("connection is already bound to another user");
            }
            return std::nullopt;
        }

        const auto current = activeConnections_.find(userId);
        std::optional<ConnectionId> replaced;
        // Complete allocations before changing either side of an existing binding.
        UserId boundUser = userId;
        if (current == activeConnections_.end())
        {
            activeConnections_.emplace(boundUser, connectionId);
        }
        else
        {
            replaced = current->second;
            current->second = connectionId;
        }
        connection->second = std::move(boundUser);
        if (replaced)
        {
            connections_.erase(*replaced);
            const std::lock_guard eventsLock(eventsMutex_);
            events_.erase(*replaced);
        }
        lock.unlock();
        if (replaced) sendParticipantsSnapshot(userId);
        else publishParticipantsChanged();
        return replaced;
    }

    std::optional<UserId> ProtocolManager::getUserId(ConnectionId connectionId) const
    {
        requireHostMode();
        const std::lock_guard lock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end() || !connection->second)
        {
            return std::nullopt;
        }
        const auto active = activeConnections_.find(*connection->second);
        if (active == activeConnections_.end() || active->second != connectionId)
        {
            return std::nullopt;
        }
        return connection->second;
    }

    CommandContext ProtocolManager::makeContext(ConnectionId connectionId) const
    {
        requireHostMode();
        const std::lock_guard lock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end())
        {
            throw ProtocolError(ProtocolErrorCode::NotAdmitted, "connection is not registered or is no longer active");
        }
        if (connection->second)
        {
            const auto active = activeConnections_.find(*connection->second);
            if (active == activeConnections_.end() || active->second != connectionId)
            {
                throw ProtocolError(ProtocolErrorCode::NotAdmitted, "connection no longer matches the active user binding");
            }
        }
        return CommandContext{connection->second, connectionId};
    }

    bool ProtocolManager::closeConnection(ConnectionId connectionId)
    {
        requireHostMode();
        const std::lock_guard executionLock(executionMutex_);
        std::unique_lock lock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end()) return false;
        bool wasActive = false;
        if (connection->second)
        {
            const auto active = activeConnections_.find(*connection->second);
            if (active != activeConnections_.end() && active->second == connectionId)
            {
                activeConnections_.erase(active);
                wasActive = true;
            }
        }
        connections_.erase(connection);
        {
            const std::lock_guard eventsLock(eventsMutex_);
            events_.erase(connectionId);
        }
        lock.unlock();
        if (wasActive) publishParticipantsChanged();
        return true;
    }

    std::uint16_t ProtocolManager::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        requireHostMode();
        requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        if (hosting_) throw std::logic_error("hosting is already active");
        {
            const std::lock_guard eventsLock(eventsMutex_);
            acceptingEvents_ = true;
        }
        try { hosting_ = std::make_unique<Hosting>(*this, bindAddress, port); }
        catch (...)
        {
            const std::lock_guard eventsLock(eventsMutex_);
            acceptingEvents_ = false;
            events_.clear();
            throw;
        }
        return hosting_->port();
    }

    void ProtocolManager::stopHosting()
    {
        requireHostMode();
        requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        if (!hosting_) return;
        {
            const std::lock_guard eventsLock(eventsMutex_);
            acceptingEvents_ = false;
            events_.clear();
        }
        hosting_->stop();
        const auto failure = hosting_->failure();
        hosting_.reset();
        {
            const std::lock_guard executionLock(executionMutex_);
            bool changed;
            {
                const std::lock_guard connectionsLock(connectionsMutex_);
                changed = !activeConnections_.empty();
                connections_.clear();
                activeConnections_.clear();
            }
            if (changed) publishParticipantsChanged();
        }
        if (failure) std::rethrow_exception(failure);
    }

    std::future<ClientAdmission> ProtocolManager::joinSession(std::string ip, std::uint16_t port, SessionCode code)
    {
        return startClient(std::move(ip), port, std::move(code), std::nullopt);
    }

    std::future<ClientAdmission> ProtocolManager::resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token)
    {
        return startClient(std::move(ip), port, std::move(code), std::move(token));
    }

    std::future<ClientAdmission> ProtocolManager::startClient(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token)
    {
        requireClientMode();
        requireOutsideDispatch(*this);
        const std::lock_guard lock(clientMutex_);
        if (client_)
        {
            if (client_->snapshot().state != ClientConnectionState::Disconnected) throw std::logic_error("client connection is already active");
            client_->stop();
            client_.reset();
        }
        if (nextClientRequestId_ == 0) throw std::overflow_error("client request ID space is exhausted");
        const std::lock_guard executionLock(executionMutex_);
        auto connection = std::make_shared<ClientConnection>(*this, nextClientRequestId_, std::move(ip), port, std::move(code), std::move(token), "client-" + std::to_string(nextClientRequestId_));
        auto result = connection->takeResult();
        client_ = std::move(connection);
        activeClient_ = client_;
        ++nextClientRequestId_;
        try { client_->start(); }
        catch (...)
        {
            activeClient_.reset();
            client_.reset();
            throw;
        }
        return result;
    }

    void ProtocolManager::disconnectFromHost()
    {
        requireClientMode();
        requireOutsideDispatch(*this);
        const std::lock_guard lock(clientMutex_);
        if (client_) client_->stop();
        const std::lock_guard executionLock(executionMutex_);
        activeClient_.reset();
        client_.reset();
    }

    ClientSnapshot ProtocolManager::getClientState() const
    {
        requireClientMode();
        const std::lock_guard lock(executionMutex_);
        const auto connection = activeClient_.lock();
        return connection ? connection->snapshot() : ClientSnapshot{};
    }
}
