#include "golarion/play/permission_manager.hpp"
#include "golarion/play/display_name.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::array<std::string_view, 14> FeatureNames{
        "permissions.registerUser", "permissions.getHostId", "permissions.getPermissions", "permissions.hasPermission", "permissions.hasPermissions",
        "permissions.updatePermissions", "permissions.applyPreset", "permissions.toSaveData", "permissions.restoreSaveData", "permissions.registerUserWithCommit",
        "permissions.applyPresetWithCommit",
        "permissions.getDisplayName", "permissions.setDisplayNameWithCommit", "permissions.updatePermissionsWithCommit"
    };

    void validateUserId(const golarion::play::UserId &userId)
    {
        if (userId.empty() || std::any_of(userId.begin(), userId.end(), [](unsigned char value) { return value <= 32 || value == 127; }))
        {
            throw std::invalid_argument("user ID must not be empty or contain ASCII whitespace/control characters");
        }
    }

}

namespace golarion::play
{
    std::vector<UserPermissionsSaveData> PermissionManager::saveUsers(const Users &users)
    {
        std::vector<UserPermissionsSaveData> result;
        result.reserve(users.size());
        for (const auto &[userId, state] : users) result.push_back({userId, state.permissions, state.displayName});
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; });
        return result;
    }
    PermissionManager::PermissionManager(ResourceManager &resources, UserId hostId) : resources_(resources), hostId_(std::move(hostId))
    {
        validateUserId(hostId_);
        users_.emplace(hostId_, UserState{PermissionSet::all(), {}});
        try
        {
            resources_.registerFeature<RegisterUserSignature>(FeatureNames[0], [this](UserId userId) { registerUser(std::move(userId)); });
            ++registeredFeatureCount_;
            resources_.registerFeature<GetHostIdSignature>(FeatureNames[1], [this] { return getHostId(); });
            ++registeredFeatureCount_;
            resources_.registerFeature<GetPermissionsSignature>(FeatureNames[2], [this](const UserId &userId) { return getPermissions(userId); });
            ++registeredFeatureCount_;
            resources_.registerFeature<HasPermissionSignature>(FeatureNames[3], [this](const UserId &userId, Permission permission) { return hasPermission(userId, permission); });
            ++registeredFeatureCount_;
            resources_.registerFeature<HasPermissionsSignature>(FeatureNames[4], [this](const UserId &userId, PermissionSet required) { return hasPermissions(userId, required); });
            ++registeredFeatureCount_;
            resources_.registerFeature<UpdatePermissionsSignature>(FeatureNames[5], [this](const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke) { updatePermissions(actorId, targetId, grant, revoke); });
            ++registeredFeatureCount_;
            resources_.registerFeature<ApplyPresetSignature>(FeatureNames[6], [this](const UserId &actorId, const UserId &targetId, PermissionPreset preset) { applyPreset(actorId, targetId, preset); });
            ++registeredFeatureCount_;
            resources_.registerFeature<ToSaveDataSignature>(FeatureNames[7], [this] { return toSaveData(); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RestoreSaveDataSignature>(FeatureNames[8], [this](const std::vector<UserPermissionsSaveData> &data) { restoreSaveData(data); });
            ++registeredFeatureCount_;
            resources_.registerFeature<RegisterUserWithCommitSignature>(FeatureNames[9], [this](UserId userId, RegistrationCommit commit, std::string name) { registerUserWithCommit(std::move(userId), commit, std::move(name)); });
            ++registeredFeatureCount_;
            resources_.registerFeature<ApplyPresetWithCommitSignature>(FeatureNames[10], [this](const UserId &actorId, const UserId &targetId, PermissionPreset preset, PermissionsCommit commit) { return applyPresetWithCommit(actorId, targetId, preset, commit); });
            ++registeredFeatureCount_;
            resources_.registerFeature<GetDisplayNameSignature>(FeatureNames[11], [this](const UserId &userId) { return getDisplayName(userId); });
            ++registeredFeatureCount_;
            resources_.registerFeature<SetDisplayNameWithCommitSignature>(FeatureNames[12], [this](const UserId &userId, std::string name, RegistrationCommit commit) { return setDisplayNameWithCommit(userId, std::move(name), commit); });
            ++registeredFeatureCount_;
            resources_.registerFeature<UpdatePermissionsWithCommitSignature>(FeatureNames[13], [this](const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke, PermissionsCommit commit) { return updatePermissionsWithCommit(actorId, targetId, grant, revoke, commit); });
            ++registeredFeatureCount_;
        }
        catch (...)
        {
            unregisterFeatures();
            throw;
        }
    }

    PermissionManager::~PermissionManager() { unregisterFeatures(); }

    void PermissionManager::registerUser(UserId userId)
    {
        validateUserId(userId);
        const std::lock_guard lock(mutex_);
        if (!users_.emplace(std::move(userId), UserState{permissionsForPreset(PermissionPreset::Player), {}}).second) throw std::invalid_argument("user is already registered");
    }

    void PermissionManager::registerUserWithCommit(UserId userId, const RegistrationCommit &commit, std::string displayName)
    {
        validateUserId(userId);
        if (!commit) throw std::invalid_argument("registration commit callback must not be empty");
        if (!displayName.empty()) displayName = normalizeDisplayName(displayName);
        const std::lock_guard lock(mutex_);
        auto candidate = users_;
        if (!candidate.emplace(std::move(userId), UserState{permissionsForPreset(PermissionPreset::Player), std::move(displayName)}).second) throw std::invalid_argument("user is already registered");
        // Commit callbacks receive staged data and must not reenter managers or acquire their locks.
        commit(saveUsers(candidate));
        users_.swap(candidate);
    }

    std::string PermissionManager::getDisplayName(const UserId &userId) const
    {
        const std::lock_guard lock(mutex_);
        const auto user = users_.find(userId);
        return user == users_.end() ? std::string{} : user->second.displayName;
    }

    bool PermissionManager::setDisplayNameWithCommit(const UserId &userId, std::string displayName, const RegistrationCommit &commit)
    {
        displayName = normalizeDisplayName(displayName);
        if (!commit) throw std::invalid_argument("display name commit callback must not be empty");
        const std::lock_guard lock(mutex_);
        auto &name = users_.at(userId).displayName;
        if (name == displayName) return false;
        auto candidate = saveUsers(users_);
        const auto entry = std::find_if(candidate.begin(), candidate.end(), [&](const auto &user) { return user.userId == userId; });
        entry->displayName = displayName;
        commit(candidate);
        name.swap(displayName);
        return true;
    }

    UserId PermissionManager::getHostId() const { return hostId_; }

    PermissionSet PermissionManager::getPermissionsUnlocked(const UserId &userId) const
    {
        const auto user = users_.find(userId);
        if (user == users_.end()) throw std::invalid_argument("user is not registered: " + userId);
        return user->second.permissions;
    }

    PermissionSet PermissionManager::getPermissions(const UserId &userId) const
    {
        const std::lock_guard lock(mutex_);
        return getPermissionsUnlocked(userId);
    }

    bool PermissionManager::hasPermission(const UserId &userId, Permission permission) const { return hasPermissions(userId, PermissionSet(permission)); }

    bool PermissionManager::hasPermissions(const UserId &userId, PermissionSet required) const
    {
        const std::lock_guard lock(mutex_);
        return getPermissionsUnlocked(userId).hasAll(required);
    }

    void PermissionManager::requireCanModifyUnlocked(const UserId &actorId, const UserId &targetId) const
    {
        const auto actor = getPermissionsUnlocked(actorId);
        static_cast<void>(getPermissionsUnlocked(targetId));
        if (targetId == hostId_) throw PermissionDenied("the host permissions cannot be changed");
        if (actorId != hostId_ && (actorId == targetId || !actor.hasAll(Permission::ChangePermissions)))
        {
            throw PermissionDenied("actor cannot modify this participant's permissions");
        }
    }

    void PermissionManager::updatePermissions(const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke)
    {
        const std::lock_guard lock(mutex_);
        users_.at(targetId).permissions = updatedPermissionsUnlocked(actorId, targetId, grant, revoke);
    }

    PermissionSet PermissionManager::updatedPermissionsUnlocked(const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke) const
    {
        if (grant.hasAny(revoke)) throw std::invalid_argument("a permission cannot be granted and revoked together");
        requireCanModifyUnlocked(actorId, targetId);
        if (actorId != hostId_ && (grant | revoke).hasAny(Permission::ChangePermissions)) throw PermissionDenied("only the host can grant or revoke ChangePermissions");

        auto replacement = users_.at(targetId).permissions;
        replacement.add(grant);
        replacement.remove(revoke);
        return replacement;
    }

    void PermissionManager::applyPreset(const UserId &actorId, const UserId &targetId, PermissionPreset preset)
    {
        const std::lock_guard lock(mutex_);
        users_.at(targetId).permissions = presetPermissionsUnlocked(actorId, targetId, preset);
    }

    PermissionSet PermissionManager::presetPermissionsUnlocked(const UserId &actorId, const UserId &targetId, PermissionPreset preset) const
    {
        auto replacement = permissionsForPreset(preset);
        requireCanModifyUnlocked(actorId, targetId);
        const auto target = users_.at(targetId).permissions;
        if (target.hasAll(Permission::ChangePermissions)) replacement.add(Permission::ChangePermissions);
        return replacement;
    }

    PermissionSet PermissionManager::commitPermissionsUnlocked(const UserId &targetId, PermissionSet replacement, const PermissionsCommit &commit)
    {
        auto candidate = users_;
        candidate.at(targetId).permissions = replacement;
        commit(saveUsers(candidate));
        users_.swap(candidate);
        return replacement;
    }

    PermissionSet PermissionManager::applyPresetWithCommit(const UserId &actorId, const UserId &targetId, PermissionPreset preset, const PermissionsCommit &commit)
    {
        if (!commit) throw std::invalid_argument("permission commit callback must not be empty");
        const std::lock_guard lock(mutex_);
        return commitPermissionsUnlocked(targetId, presetPermissionsUnlocked(actorId, targetId, preset), commit);
    }

    PermissionSet PermissionManager::updatePermissionsWithCommit(const UserId &actorId, const UserId &targetId, PermissionSet grant, PermissionSet revoke, const PermissionsCommit &commit)
    {
        if (!commit) throw std::invalid_argument("permission commit callback must not be empty");
        const std::lock_guard lock(mutex_);
        return commitPermissionsUnlocked(targetId, updatedPermissionsUnlocked(actorId, targetId, grant, revoke), commit);
    }

    std::vector<UserPermissionsSaveData> PermissionManager::toSaveData() const
    {
        const std::lock_guard lock(mutex_);
        return saveUsers(users_);
    }

    void PermissionManager::restoreSaveData(const std::vector<UserPermissionsSaveData> &data)
    {
        Users restored;
        for (const auto &entry : data)
        {
            validateUserId(entry.userId);
            auto name = entry.displayName.empty() ? std::string{} : normalizeDisplayName(entry.displayName);
            if (!restored.emplace(entry.userId, UserState{entry.permissions, std::move(name)}).second) throw std::invalid_argument("duplicate user ID in permission data");
        }
        const auto host = restored.find(hostId_);
        if (host == restored.end() || host->second.permissions != PermissionSet::all()) throw std::invalid_argument("permission data must preserve this host and all its permissions");
        const std::lock_guard lock(mutex_);
        users_.swap(restored);
    }

    void PermissionManager::unregisterFeatures() noexcept
    {
        while (registeredFeatureCount_ > 0)
        {
            --registeredFeatureCount_;
            try { resources_.unregisterFeature(FeatureNames[registeredFeatureCount_]); }
            catch (...) { } // Destruction and constructor rollback must not throw.
        }
    }
}
