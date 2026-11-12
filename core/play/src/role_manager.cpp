#include "golarion/play/role_manager.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::array<std::string_view, 7> FeatureNames{"roles.getRole", "roles.hasAtLeastRole", "roles.setRole", "roles.registerUser", "roles.toSaveData", "roles.restoreSaveData", "roles.registerUserWithCommit"};

    std::vector<golarion::play::UserRoleSaveData> savedRoles(const std::unordered_map<golarion::play::UserId, golarion::play::Role> &roles)
    {
        std::vector<golarion::play::UserRoleSaveData> result;
        result.reserve(roles.size());
        for (const auto &[userId, role] : roles) result.push_back({userId, role});
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; });
        return result;
    }

    void validateUserId(const golarion::play::UserId &userId)
    {
        if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; }))
        {
            throw std::invalid_argument("user ID must not be empty or contain ASCII whitespace/control characters");
        }
    }

    int roleRank(golarion::play::Role role)
    {
        using golarion::play::Role;
        switch (role)
        {
            case Role::Any: return -1;
            case Role::Spectator: return 0;
            case Role::Player: return 1;
            case Role::Master: return 2;
            case Role::Host: return 3;
        }
        throw std::invalid_argument("unknown role");
    }
}

namespace golarion::play
{
    RoleManager::RoleManager(ResourceManager &resources, UserId hostId) : resources_(resources)
    {
        validateUserId(hostId);
        roles_.emplace(std::move(hostId), Role::Host);

        try
        {
            resources_.registerFeature<GetRoleSignature>(FeatureNames[0], [this](const UserId &userId) { return getRole(userId); });
            ++registeredFeatureCount_;
            resources_.registerFeature<HasAtLeastRoleSignature>(FeatureNames[1], [this](const std::optional<UserId> &userId, Role minimumRole) { return hasAtLeastRole(userId, minimumRole); });
            ++registeredFeatureCount_;
            resources_.registerFeature<SetRoleSignature>(FeatureNames[2], [this](const UserId &actorId, const UserId &targetId, Role newRole) { setRole(actorId, targetId, newRole); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RegisterUserSignature>(FeatureNames[3], [this](UserId userId) { registerUser(std::move(userId)); });
            ++registeredFeatureCount_;
            resources_.registerFeature<ToSaveDataSignature>(FeatureNames[4], [this] { return toSaveData(); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RestoreSaveDataSignature>(FeatureNames[5], [this](const std::vector<UserRoleSaveData> &data) { restoreSaveData(data); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RegisterUserWithCommitSignature>(FeatureNames[6], [this](UserId userId, RegistrationCommit commit) { registerUserWithCommit(std::move(userId), commit); });
            ++registeredFeatureCount_;
        }
        catch (...)
        {
            unregisterFeatures();
            throw;
        }
    }

    RoleManager::~RoleManager()
    {
        unregisterFeatures();
    }

    void RoleManager::registerUser(UserId userId)
    {
        validateUserId(userId);
        const std::lock_guard lock(mutex_);
        if (roles_.contains(userId))
        {
            throw std::invalid_argument("user is already registered: " + userId);
        }
        roles_.emplace(std::move(userId), Role::Player);
    }

    void RoleManager::registerUserWithCommit(UserId userId, const RegistrationCommit &commit)
    {
        validateUserId(userId);
        if (!commit) throw std::invalid_argument("registration commit callback must not be empty");
        const std::lock_guard lock(mutex_);
        auto candidate = roles_;
        if (!candidate.emplace(std::move(userId), Role::Player).second) throw std::invalid_argument("user is already registered");
        // The callback receives staged data and must not reenter RoleManager or acquire ProtocolManager locks.
        commit(savedRoles(candidate));
        roles_.swap(candidate);
    }

    Role RoleManager::getRole(const UserId &userId) const
    {
        const std::lock_guard lock(mutex_);
        return getRoleUnlocked(userId);
    }

    Role RoleManager::getRoleUnlocked(const UserId &userId) const
    {
        const auto user = roles_.find(userId);
        if (user == roles_.end())
        {
            throw std::invalid_argument("user is not registered: " + userId);
        }
        return user->second;
    }

    bool RoleManager::hasAtLeastRole(const std::optional<UserId> &userId, Role minimumRole) const
    {
        const std::lock_guard lock(mutex_);
        return roleRank(userId ? getRoleUnlocked(*userId) : Role::Any) >= roleRank(minimumRole);
    }

    void RoleManager::setRole(const UserId &actorId, const UserId &targetId, Role newRole)
    {
        static_cast<void>(roleRank(newRole));
        if (newRole == Role::Any) throw std::invalid_argument("Any is an access threshold, not an assignable participant role");
        const std::lock_guard lock(mutex_);
        const Role actorRole = getRoleUnlocked(actorId);
        const Role targetRole = getRoleUnlocked(targetId);
        if (targetRole == Role::Host || newRole == Role::Host)
        {
            throw std::invalid_argument("the host role cannot be changed or assigned");
        }
        if (actorRole != Role::Host && (actorRole != Role::Master || targetRole == Role::Master || newRole == Role::Master))
        {
            throw std::invalid_argument("actor is not allowed to make this role change");
        }
        roles_.at(targetId) = newRole;
    }

    std::vector<UserRoleSaveData> RoleManager::toSaveData() const
    {
        const std::lock_guard lock(mutex_);
        return savedRoles(roles_);
    }

    void RoleManager::restoreSaveData(const std::vector<UserRoleSaveData> &data)
    {
        const std::lock_guard lock(mutex_);
        std::unordered_map<UserId, Role> restored;
        const auto host = std::find_if(roles_.begin(), roles_.end(), [](const auto &entry) { return entry.second == Role::Host; });
        std::size_t hostCount = 0;
        for (const auto &entry : data)
        {
            validateUserId(entry.userId);
            static_cast<void>(roleRank(entry.role));
            if (entry.role == Role::Any) throw std::invalid_argument("Any cannot be restored as a participant role");
            if (!restored.emplace(entry.userId, entry.role).second) throw std::invalid_argument("duplicate user ID in role data");
            if (entry.role == Role::Host)
            {
                ++hostCount;
                if (entry.userId != host->first) throw std::invalid_argument("saved host does not match this Session's host");
            }
        }
        if (hostCount != 1) throw std::invalid_argument("role data must contain exactly one host");
        roles_.swap(restored);
    }

    void RoleManager::unregisterFeatures() noexcept
    {
        while (registeredFeatureCount_ > 0)
        {
            --registeredFeatureCount_;
            try { resources_.unregisterFeature(FeatureNames[registeredFeatureCount_]); }
            catch (...) { } // Destruction and constructor rollback must not throw.
        }
    }
}
