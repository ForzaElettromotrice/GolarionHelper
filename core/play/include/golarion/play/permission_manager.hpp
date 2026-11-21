#pragma once

#include "golarion/play/permission_set.hpp"
#include "golarion/play/permission_error.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/user_id.hpp"

#include <cstddef>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace golarion::play
{
    struct UserPermissionsSaveData
    {
        UserId userId;
        PermissionSet permissions;
        std::string displayName = {};
    };

    class PermissionManager final
    {
    public:
        using RegisterUserSignature = void(UserId);
        using GetHostIdSignature = UserId();
        using GetPermissionsSignature = PermissionSet(const UserId &);
        using HasPermissionSignature = bool(const UserId &, Permission);
        using HasPermissionsSignature = bool(const UserId &, PermissionSet);
        using SetPermissionSignature = void(const UserId &, const UserId &, Permission, bool);
        using ApplyPresetSignature = void(const UserId &, const UserId &, PermissionPreset);
        using ToSaveDataSignature = std::vector<UserPermissionsSaveData>();
        using RestoreSaveDataSignature = void(const std::vector<UserPermissionsSaveData> &);
        using RegistrationCommit = std::function<void(const std::vector<UserPermissionsSaveData> &)>;
        using RegisterUserWithCommitSignature = void(UserId, RegistrationCommit, std::string);
        using GetDisplayNameSignature = std::string(const UserId &);
        using SetDisplayNameWithCommitSignature = bool(const UserId &, std::string, RegistrationCommit);
        using PermissionsCommit = RegistrationCommit;
        using SetPermissionWithCommitSignature = PermissionSet(const UserId &, const UserId &, Permission, bool, PermissionsCommit);
        using ApplyPresetWithCommitSignature = PermissionSet(const UserId &, const UserId &, PermissionPreset, PermissionsCommit);
        using UpdatePermissionsWithCommitSignature = PermissionSet(const UserId &, const UserId &, PermissionSet, PermissionSet, PermissionsCommit);

        PermissionManager(ResourceManager &resources, UserId hostId);
        ~PermissionManager();
        PermissionManager(const PermissionManager &) = delete;
        PermissionManager &operator=(const PermissionManager &) = delete;
        PermissionManager(PermissionManager &&) = delete;
        PermissionManager &operator=(PermissionManager &&) = delete;

        void registerUser(UserId userId);
        void registerUserWithCommit(UserId userId, const RegistrationCommit &commit, std::string displayName = {});
        void registerCommands();
        UserId getHostId() const;
        PermissionSet getPermissions(const UserId &userId) const;
        bool hasPermission(const UserId &userId, Permission permission) const;
        bool hasPermissions(const UserId &userId, PermissionSet required) const;
        void setPermission(const UserId &actorId, const UserId &targetId, Permission permission, bool enabled);
        void applyPreset(const UserId &actorId, const UserId &targetId, PermissionPreset preset);
        PermissionSet setPermissionWithCommit(const UserId &actorId, const UserId &targetId, Permission permission, bool enabled, const PermissionsCommit &commit);
        PermissionSet applyPresetWithCommit(const UserId &actorId, const UserId &targetId, PermissionPreset preset, const PermissionsCommit &commit);
        std::vector<UserPermissionsSaveData> toSaveData() const;
        void restoreSaveData(const std::vector<UserPermissionsSaveData> &data);

    private:
        struct UserState
        {
            PermissionSet permissions;
            std::string displayName;
        };
        using Users = std::unordered_map<UserId, UserState>;

        static std::vector<UserPermissionsSaveData> saveUsers(const Users &users);
        std::string getDisplayName(const UserId &userId) const;
        bool setDisplayNameWithCommit(const UserId &userId, std::string displayName, const RegistrationCommit &commit);
        PermissionSet getPermissionsUnlocked(const UserId &userId) const;
        void requireCanModifyUnlocked(const UserId &actorId, const UserId &targetId) const;
        PermissionSet changedPermissionUnlocked(const UserId &actorId, const UserId &targetId, Permission permission, bool enabled) const;
        PermissionSet presetPermissionsUnlocked(const UserId &actorId, const UserId &targetId, PermissionPreset preset) const;
        PermissionSet commitPermissionsUnlocked(const UserId &targetId, PermissionSet replacement, const PermissionsCommit &commit);
        PermissionSet updatePermissionsWithCommit(const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke, const PermissionsCommit &commit);
        void unregisterFeatures() noexcept;

        ResourceManager &resources_;
        const UserId hostId_;
        mutable std::mutex mutex_;
        Users users_;
        std::size_t registeredFeatureCount_ = 0;
    };
}
