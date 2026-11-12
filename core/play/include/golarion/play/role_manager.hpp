#pragma once

#include "golarion/play/resource_manager.hpp"

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace golarion::play
{
    using UserId = std::string;

    enum class Role
    {
        Any = -1,
        Spectator,
        Player,
        Master,
        Host
    };

    struct UserRoleSaveData
    {
        UserId userId;
        Role role;
    };

    class RoleManager final
    {
    public:
        using RegisterUserSignature = void(UserId);
        using GetRoleSignature = Role(const UserId &);
        using HasAtLeastRoleSignature = bool(const std::optional<UserId> &, Role);
        using SetRoleSignature = void(const UserId &, const UserId &, Role);
        using ToSaveDataSignature = std::vector<UserRoleSaveData>();
        using RestoreSaveDataSignature = void(const std::vector<UserRoleSaveData> &);
        using RegistrationCommit = std::function<void(const std::vector<UserRoleSaveData> &)>;
        using RegisterUserWithCommitSignature = void(UserId, RegistrationCommit);

        RoleManager(ResourceManager &resources, UserId hostId);
        ~RoleManager();
        RoleManager(const RoleManager &) = delete;
        RoleManager &operator=(const RoleManager &) = delete;
        RoleManager(RoleManager &&) = delete;
        RoleManager &operator=(RoleManager &&) = delete;

        void registerUser(UserId userId);
        void registerUserWithCommit(UserId userId, const RegistrationCommit &commit);
        Role getRole(const UserId &userId) const;
        bool hasAtLeastRole(const std::optional<UserId> &userId, Role minimumRole) const;
        void setRole(const UserId &actorId, const UserId &targetId, Role newRole);
        std::vector<UserRoleSaveData> toSaveData() const;
        void restoreSaveData(const std::vector<UserRoleSaveData> &data);

    private:
        Role getRoleUnlocked(const UserId &userId) const;
        void unregisterFeatures() noexcept;

        ResourceManager &resources_;
        mutable std::mutex mutex_;
        std::unordered_map<UserId, Role> roles_;
        std::size_t registeredFeatureCount_ = 0;
    };
}
