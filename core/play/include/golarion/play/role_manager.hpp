#pragma once

#include "golarion/play/resource_manager.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace golarion::play
{
    using UserId = std::string;

    enum class Role
    {
        Spectator,
        Player,
        Master,
        Host
    };

    class RoleManager final
    {
    public:
        using RegisterUserSignature = void(UserId);
        using GetRoleSignature = Role(const UserId &);
        using HasAtLeastRoleSignature = bool(const UserId &, Role);
        using SetRoleSignature = void(const UserId &, const UserId &, Role);

        RoleManager(ResourceManager &resources, UserId hostId);
        ~RoleManager();
        RoleManager(const RoleManager &) = delete;
        RoleManager &operator=(const RoleManager &) = delete;
        RoleManager(RoleManager &&) = delete;
        RoleManager &operator=(RoleManager &&) = delete;

        void registerUser(UserId userId);
        Role getRole(const UserId &userId) const;
        bool hasAtLeastRole(const UserId &userId, Role minimumRole) const;
        void setRole(const UserId &actorId, const UserId &targetId, Role newRole);

    private:
        void unregisterFeatures() noexcept;

        ResourceManager &resources_;
        std::unordered_map<UserId, Role> roles_;
        std::size_t registeredFeatureCount_ = 0;
    };
}
