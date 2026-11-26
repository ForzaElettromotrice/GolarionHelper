#include "golarion/play/server_protocol.hpp"
#include "hosting.hpp"
#include "dispatch_support.hpp"
#include "local_commands.hpp"
#include "identity_storage.hpp"
#include "identity_transaction.hpp"
#include "permission_json.hpp"
#include "golarion/play/display_name.hpp"
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

    void validateUserId(const golarion::play::UserId &userId)
    {
        if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; }))
        {
            throw std::invalid_argument("user ID must not be empty or contain ASCII whitespace/control characters");
        }
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
    ServerProtocol::ServerProtocol(ResourceManager &resources, std::optional<SessionCode> hostCode)
        : resources_(resources), sessionCode_(hostCode ? *hostCode : SessionCode::generate()), userTokens_(std::make_unique<detail::UserTokens>())
    {
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

    ServerProtocol::~ServerProtocol()
    {
        shutdown();
        try { resources_.unregisterFeature(AddCommandFeature); }
        catch (...) { } // Destruction must not throw if registration was already removed.
        try { resources_.unregisterFeature(SendEventFeature); }
        catch (...) { }
        try { resources_.unregisterFeature(detail::UpdateUserPermissionsFeature); }
        catch (...) { }
    }

    void ServerProtocol::shutdown() noexcept
    {
        try
        {
            stopHosting();
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

    std::future<nlohmann::json> ServerProtocol::sendCommand(std::string name, nlohmann::json payload)
    {
        // A callback must keep its existing actor and use nested dispatch instead.
        // Queuing and waiting here would also deadlock the local worker.
        if (detail::DispatchScope<ServerProtocol>::contains(*this)) throw std::logic_error("sendCommand is not allowed from a command callback");
        name = detail::normalizedName(name);
        if (!payload.is_object()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "command payload must be a JSON object");
        const std::lock_guard lock(localCommandsMutex_);
        if (!localCommands_) localCommands_ = std::make_unique<LocalCommands>(*this);
        return localCommands_->submit(std::move(name), std::move(payload));
    }

    void ServerProtocol::add_command(std::string_view name, CommandRequirements requirements, CommandCallback callback)
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = detail::normalizedName(name);
        validateRequirements(requirements);
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

    nlohmann::json ServerProtocol::dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = detail::normalizedName(name);
        if (!payload.is_object())
        {
            throw ProtocolError(ProtocolErrorCode::InvalidRequest, "command payload must be a JSON object");
        }
        if ((!context.userId && !context.connectionId) || (context.userId && context.userId->empty()))
        {
            throw ProtocolError(ProtocolErrorCode::NotAdmitted, "command context must identify a live connection or a recognized local user");
        }
        if (context.connectionId && makeContext(*context.connectionId).userId != context.userId)
        {
            throw ProtocolError(ProtocolErrorCode::NotAdmitted, "command context no longer matches the active connection");
        }
        const auto entry = commands_.find(key);
        if (entry == commands_.end())
        {
            throw ProtocolError(ProtocolErrorCode::UnknownCommand, "command is not registered: " + key);
        }
        const Command &command = entry->second;
        bool authorized = command.requirements.access == CommandAccess::Public;
        if (context.userId)
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
        const detail::DispatchScope scope(*this);
        return command.callback(context, payload);
    }

    bool ServerProtocol::sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload)
    {
        validateUserId(userId);
        auto key = detail::normalizedName(name);
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

    std::optional<std::vector<std::uint8_t>> ServerProtocol::takeEvent(ConnectionId connectionId)
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

    bool ServerProtocol::eventQueueOverflowed(ConnectionId connectionId) const
    {
        const std::lock_guard lock(eventsMutex_);
        const auto entry = events_.find(connectionId);
        return entry != events_.end() && entry->second.overflowed;
    }

    std::string ServerProtocol::getSessionCode() const
    {
        return std::string(hostCode().value());
    }

    const SessionCode &ServerProtocol::hostCode() const
    {
        return sessionCode_;
    }

    bool ServerProtocol::verifySessionCode(std::string_view candidate) const noexcept
    {
        return sessionCode_.matches(candidate);
    }

    std::string ServerProtocol::issueUserToken(const UserId &userId)
    {
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

    std::optional<UserId> ServerProtocol::resolveUserToken(std::string_view token) const
    {
        const std::lock_guard lock(identityMutex_);
        return userTokens_->resolve(token);
    }

    PermissionSet ServerProtocol::updateUserPermissions(const std::function<PermissionSet(const PermissionManager::PermissionsCommit &)> &mutation)
    {
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

    ServerProtocol::HostingSnapshot ServerProtocol::getHostingSnapshot() const
    {
        detail::requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        const auto port = hosting_ ? std::make_optional(hosting_->port()) : std::nullopt;
        return {port, getParticipants()};
    }

    IdentitySaveData ServerProtocol::identityDataLocked() const
    {
        return {
            resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId"),
            resources_.invoke<PermissionManager::ToSaveDataSignature>("permissions.toSaveData"),
            userTokens_->toSaveData()
        };
    }

    bool ServerProtocol::commitDisplayNameLocked(const UserId &userId, std::string displayName)
    {
        const auto hostId = resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId");
        return resources_.invoke<PermissionManager::SetDisplayNameWithCommitSignature>("permissions.setDisplayNameWithCommit", userId, std::move(displayName),
            [this, &hostId](const std::vector<UserPermissionsSaveData> &users)
            {
                if (!identityArchivePath_) return;
                detail::writeIdentityState(*identityArchivePath_, IdentitySaveData{hostId, users, userTokens_->toSaveData()});
            });
    }

    void ServerProtocol::setHostDisplayName(std::string displayName)
    {
        const std::lock_guard executionLock(executionMutex_);
        const auto hostId = resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId");
        bool changed;
        {
            const std::lock_guard identityLock(identityMutex_);
            changed = commitDisplayNameLocked(hostId, std::move(displayName));
        }
        if (changed) publishParticipantsChanged();
    }

    IdentitySaveData ServerProtocol::toIdentityData() const
    {
        const std::lock_guard lock(identityMutex_);
        return identityDataLocked();
    }

    void ServerProtocol::saveIdentityState(const std::filesystem::path &path)
    {
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

    void ServerProtocol::loadIdentityState(const std::filesystem::path &path)
    {
        detail::requireOutsideDispatch(*this);
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

    ConnectionId ServerProtocol::registerConnection()
    {
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

    std::optional<ConnectionId> ServerProtocol::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        return bindUser(connectionId, userId, std::nullopt);
    }

    std::optional<ConnectionId> ServerProtocol::bindUser(ConnectionId connectionId, const UserId &userId, std::optional<std::string> displayName)
    {
        validateUserId(userId);
        const std::lock_guard executionLock(executionMutex_);
        std::unique_lock identityLock(identityMutex_, std::defer_lock);
        if (displayName) identityLock.lock();
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
        const bool newBinding = current == activeConnections_.end();
        if (newBinding) activeConnections_.emplace(boundUser, connectionId);
        bool nameChanged = false;
        try
        {
            if (displayName) nameChanged = commitDisplayNameLocked(userId, std::move(*displayName));
        }
        catch (...)
        {
            if (newBinding) activeConnections_.erase(boundUser);
            throw;
        }
        if (!newBinding)
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
        if (identityLock.owns_lock()) identityLock.unlock();
        if (newBinding || nameChanged) publishParticipantsChanged();
        else sendParticipantsSnapshot(userId);
        return replaced;
    }

    std::optional<UserId> ServerProtocol::getUserId(ConnectionId connectionId) const
    {
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

    CommandContext ServerProtocol::makeContext(ConnectionId connectionId) const
    {
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

    bool ServerProtocol::closeConnection(ConnectionId connectionId)
    {
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

    std::uint16_t ServerProtocol::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        detail::requireOutsideDispatch(*this);
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

    void ServerProtocol::stopHosting()
    {
        detail::requireOutsideDispatch(*this);
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

}
