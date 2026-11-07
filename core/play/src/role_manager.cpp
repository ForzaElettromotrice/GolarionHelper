#include "golarion/play/role_manager.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::array<std::string_view, 4> FeatureNames{"roles.getRole", "roles.hasAtLeastRole", "roles.setRole", "roles.registerUser"};

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
            resources_.registerFeature<HasAtLeastRoleSignature>(FeatureNames[1], [this](const UserId &userId, Role minimumRole) { return hasAtLeastRole(userId, minimumRole); });
            ++registeredFeatureCount_;
            resources_.registerFeature<SetRoleSignature>(FeatureNames[2], [this](const UserId &actorId, const UserId &targetId, Role newRole) { setRole(actorId, targetId, newRole); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RegisterUserSignature>(FeatureNames[3], [this](UserId userId) { registerUser(std::move(userId)); });
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
        if (roles_.contains(userId))
        {
            throw std::invalid_argument("user is already registered: " + userId);
        }
        roles_.emplace(std::move(userId), Role::Player);
    }

    Role RoleManager::getRole(const UserId &userId) const
    {
        const auto user = roles_.find(userId);
        if (user == roles_.end())
        {
            throw std::invalid_argument("user is not registered: " + userId);
        }
        return user->second;
    }

    bool RoleManager::hasAtLeastRole(const UserId &userId, Role minimumRole) const
    {
        return roleRank(getRole(userId)) >= roleRank(minimumRole);
    }

    void RoleManager::setRole(const UserId &actorId, const UserId &targetId, Role newRole)
    {
        static_cast<void>(roleRank(newRole));
        const Role actorRole = getRole(actorId);
        const Role targetRole = getRole(targetId);
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
