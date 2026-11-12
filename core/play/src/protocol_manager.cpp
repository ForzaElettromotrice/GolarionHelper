#include "golarion/play/protocol_manager.hpp"
#include "hosting.hpp"
#include "identity_storage.hpp"
#include "user_tokens.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view AddCommandFeature = "protocol.add_command";

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
        if (DispatchScope::contains(owner)) throw std::logic_error("hosting lifecycle and identity restoration are not allowed from a command callback");
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

    void validateMinimumRole(golarion::play::Role role)
    {
        using golarion::play::Role;
        switch (role)
        {
            case Role::Any:
            case Role::Spectator:
            case Role::Player:
            case Role::Master:
            case Role::Host:
                return;
        }
        throw std::invalid_argument("unknown minimum role");
    }
}

namespace golarion::play
{
    ProtocolManager::ProtocolManager(ResourceManager &resources) : resources_(resources), userTokens_(std::make_unique<detail::UserTokens>())
    {
        registerAdmissionCommands();
        resources_.registerFeature<AddCommandSignature>(AddCommandFeature, [this](std::string_view name, Role minimumRole, CommandCallback callback)
        {
            add_command(name, minimumRole, std::move(callback));
        });
    }

    ProtocolManager::~ProtocolManager()
    {
        try { stopHosting(); }
        catch (...) { } // Destruction must not propagate background worker errors.
        try { resources_.unregisterFeature(AddCommandFeature); }
        catch (...) { } // Destruction must not throw if registration was already removed.
    }

    void ProtocolManager::add_command(std::string_view name, Role minimumRole, CommandCallback callback)
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = normalizedName(name);
        validateMinimumRole(minimumRole);
        if (!callback)
        {
            throw std::invalid_argument("command callback must not be empty: " + key);
        }
        if (commands_.contains(key))
        {
            throw std::invalid_argument("command is already registered: " + key);
        }
        commands_.emplace(key, Command{minimumRole, std::move(callback)});
    }

    nlohmann::json ProtocolManager::dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::string key = normalizedName(name);
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
        bool authorized;
        try { authorized = resources_.invoke<RoleManager::HasAtLeastRoleSignature>("roles.hasAtLeastRole", context.userId, command.minimumRole); }
        catch (...)
        {
            throw ProtocolError(ProtocolErrorCode::CommandFailed, "cannot verify the command role requirement");
        }
        if (!authorized)
        {
            throw ProtocolError(ProtocolErrorCode::Forbidden, "user does not have the required role for command: " + key);
        }
        const DispatchScope scope(*this);
        return command.callback(context, payload);
    }

    std::string ProtocolManager::getSessionCode() const
    {
        return std::string(sessionCode_.value());
    }

    bool ProtocolManager::verifySessionCode(std::string_view candidate) const noexcept
    {
        return sessionCode_.matches(candidate);
    }

    std::string ProtocolManager::issueUserToken(const UserId &userId)
    {
        const std::lock_guard lock(identityMutex_);
        static_cast<void>(resources_.invoke<RoleManager::GetRoleSignature>("roles.getRole", userId));
        if (!identityArchivePath_) throw std::logic_error("save or load an identity archive before issuing tokens");
        auto candidate = std::make_unique<detail::UserTokens>(*userTokens_);
        auto token = candidate->issue(userId);
        const IdentitySaveData data{resources_.invoke<RoleManager::ToSaveDataSignature>("roles.toSaveData"), candidate->toSaveData()};
        detail::writeIdentityState(*identityArchivePath_, data);
        userTokens_.swap(candidate);
        return token;
    }

    std::optional<UserId> ProtocolManager::resolveUserToken(std::string_view token) const
    {
        const std::lock_guard lock(identityMutex_);
        return userTokens_->resolve(token);
    }

    void ProtocolManager::saveIdentityState(const std::filesystem::path &path)
    {
        if (path.empty()) throw std::invalid_argument("identity archive path must not be empty");
        std::optional<std::filesystem::path> destination(std::filesystem::absolute(path).lexically_normal());
        const std::lock_guard lock(identityMutex_);
        if (std::filesystem::exists(*destination) && (!identityArchivePath_ || *identityArchivePath_ != *destination))
        {
            throw std::logic_error("load an existing identity archive before replacing it");
        }
        const IdentitySaveData data{resources_.invoke<RoleManager::ToSaveDataSignature>("roles.toSaveData"), userTokens_->toSaveData()};
        detail::writeIdentityState(*destination, data);
        identityArchivePath_.swap(destination);
    }

    void ProtocolManager::loadIdentityState(const std::filesystem::path &path)
    {
        requireOutsideDispatch(*this);
        if (path.empty()) throw std::invalid_argument("identity archive path must not be empty");
        std::optional<std::filesystem::path> source(std::filesystem::absolute(path).lexically_normal());
        const std::lock_guard hostingLock(hostingMutex_);
        if (hosting_) throw std::logic_error("stop hosting before restoring identities");
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard identityLock(identityMutex_);
        const auto data = detail::readIdentityState(*source);
        auto candidate = std::make_unique<detail::UserTokens>(detail::UserTokens::fromSaveData(data.tokens));
        const std::lock_guard connectionsLock(connectionsMutex_);
        resources_.invoke<RoleManager::RestoreSaveDataSignature>("roles.restoreSaveData", data.users);
        userTokens_.swap(candidate);
        identityArchivePath_.swap(source);
        connections_.clear();
        activeConnections_.clear();
    }

    ConnectionId ProtocolManager::registerConnection()
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

    std::optional<ConnectionId> ProtocolManager::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        validateUserId(userId);
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard lock(connectionsMutex_);
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
        if (replaced) connections_.erase(*replaced);
        return replaced;
    }

    std::optional<UserId> ProtocolManager::getUserId(ConnectionId connectionId) const
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

    CommandContext ProtocolManager::makeContext(ConnectionId connectionId) const
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

    bool ProtocolManager::closeConnection(ConnectionId connectionId)
    {
        const std::lock_guard executionLock(executionMutex_);
        const std::lock_guard lock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end()) return false;
        if (connection->second)
        {
            const auto active = activeConnections_.find(*connection->second);
            if (active != activeConnections_.end() && active->second == connectionId)
            {
                activeConnections_.erase(active);
            }
        }
        connections_.erase(connection);
        return true;
    }

    std::uint16_t ProtocolManager::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        if (hosting_) throw std::logic_error("hosting is already active");
        hosting_ = std::make_unique<Hosting>(*this, bindAddress, port);
        return hosting_->port();
    }

    void ProtocolManager::stopHosting()
    {
        requireOutsideDispatch(*this);
        const std::lock_guard lock(hostingMutex_);
        if (!hosting_) return;
        hosting_->stop();
        const auto failure = hosting_->failure();
        hosting_.reset();
        {
            const std::lock_guard executionLock(executionMutex_);
            const std::lock_guard connectionsLock(connectionsMutex_);
            connections_.clear();
            activeConnections_.clear();
        }
        if (failure) std::rethrow_exception(failure);
    }
}
