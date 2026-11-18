#include "golarion/play/permission_manager.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace
{
    using namespace golarion::play;

    constexpr std::array<Permission, 6> Flags{Permission::Permission1, Permission::Permission2, Permission::Permission3, Permission::Permission4, Permission::Permission5, Permission::ChangePermissions};
    constexpr std::array<std::string_view, 12> Features{
        "permissions.registerUser", "permissions.getHostId", "permissions.getPermissions", "permissions.hasPermission", "permissions.hasPermissions",
        "permissions.setPermission", "permissions.applyPreset", "permissions.toSaveData", "permissions.restoreSaveData", "permissions.registerUserWithCommit",
        "permissions.setPermissionWithCommit", "permissions.applyPresetWithCommit"
    };

    void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

    template<typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    bool equal(const std::vector<UserPermissionsSaveData> &left, const std::vector<UserPermissionsSaveData> &right)
    {
        return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](const auto &a, const auto &b) { return a.userId == b.userId && a.permissions == b.permissions; });
    }

    PermissionSet combination(unsigned index)
    {
        PermissionSet result;
        for (std::size_t flag = 0; flag < Flags.size(); ++flag) if ((index >> flag) % 2 != 0) result.add(Flags[flag]);
        return result;
    }

    void bootstrapAndDetachedChecks()
    {
        static_assert(!std::is_copy_constructible_v<PermissionManager> && !std::is_move_constructible_v<PermissionManager>);
        ResourceManager resources;
        PermissionManager permissions(resources, "real-host");
        check(permissions.getHostId() == "real-host" && permissions.getPermissions("real-host") == PermissionSet::all(), "host identity or permissions are wrong");
        resources.invoke<PermissionManager::RegisterUserSignature>("permissions.registerUser", "alice");
        permissions.registerUser("Alice");
        check(permissions.getPermissions("alice") == permissionsForPreset(PermissionPreset::Player), "new user did not receive Player permissions");
        check(!permissions.hasPermission("alice", Permission::ChangePermissions), "registration granted administrative permissions");
        check(permissions.hasPermissions("alice", {}) && !permissions.hasPermissions("alice", PermissionSet::all()), "empty/all permission checks are wrong");
        auto copy = permissions.getPermissions("alice");
        copy.add(Permission::Permission5);
        check(!permissions.hasPermission("alice", Permission::Permission5), "returned permissions alias internal state");
        const auto initial = permissions.toSaveData();
        check(rejects([&] { permissions.registerUser("alice"); }), "duplicate user accepted");
        check(rejects([&] { permissions.registerUser("real-host"); }), "duplicate host accepted");
        for (const UserId invalid : {"", " ", "bad id", "bad\tid", "bad\nid", "bad\x7f"})
        {
            check(rejects([&] { permissions.registerUser(invalid); }), "invalid user ID accepted");
            ResourceManager empty;
            check(rejects([&] { PermissionManager manager(empty, invalid); }), "invalid host ID accepted");
            empty.registerFeature<void()>(Features.front(), [] {});
        }
        check(rejects([&] { permissions.getPermissions("unknown"); }), "unknown user returned permissions");
        check(rejects([&] { permissions.hasPermissions("unknown", {}); }), "unknown user passed an empty requirement");
        check(rejects([&] { permissions.hasPermission("alice", static_cast<Permission>(64)); }), "unknown permission accepted");
        check(equal(initial, permissions.toSaveData()), "failed validation changed users");
    }

    void completeMutationPolicy()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        for (unsigned actorFlags = 0; actorFlags < 64; ++actorFlags)
        {
            for (const bool targetCanModify : {false, true})
            {
                for (const auto permission : Flags)
                {
                    for (const bool enabled : {false, true})
                    {
                        for (const UserId target : {"target", "actor", "host"})
                        {
                            const auto actor = combination(actorFlags);
                            auto targetPermissions = permissionsForPreset(PermissionPreset::Player);
                            if (targetCanModify) targetPermissions.add(Permission::ChangePermissions);
                            permissions.restoreSaveData({{"host", PermissionSet::all()}, {"actor", actor}, {"target", targetPermissions}});
                            const auto before = permissions.toSaveData();
                            const bool expected = actor.hasAll(Permission::ChangePermissions) && target == "target" && permission != Permission::ChangePermissions;
                            const bool accepted = !rejects([&] { permissions.setPermission("actor", target, permission, enabled); });
                            check(accepted == expected, "delegate mutation policy mismatch");
                            if (!accepted) check(equal(before, permissions.toSaveData()), "denied mutation changed data");
                            else
                            {
                                auto after = targetPermissions;
                                if (enabled) after.add(permission);
                                else after.remove(permission);
                                check(permissions.getPermissions("target") == after && permissions.getPermissions("actor") == actor, "mutation affected the wrong flags or participant");
                            }
                            check(permissions.getPermissions("host") == PermissionSet::all(), "host permissions changed");
                        }
                    }
                }
            }
        }
        permissions.registerUser("ordinary");
        for (const auto permission : Flags)
        {
            for (const bool enabled : {false, true})
            {
                permissions.setPermission("host", "ordinary", permission, enabled);
                check(permissions.hasPermission("ordinary", permission) == enabled, "host cannot freely assign/revoke a permission");
                check(rejects([&] { permissions.setPermission("host", "host", permission, enabled); }), "host permissions are mutable");
            }
        }
        const auto before = permissions.toSaveData();
        check(rejects([&] { permissions.setPermission("missing", "ordinary", Permission::Permission1, true); }), "unknown actor changed permissions");
        check(rejects([&] { permissions.setPermission("host", "missing", Permission::Permission1, true); }), "unknown target changed permissions");
        check(rejects([&] { permissions.setPermission("host", "ordinary", static_cast<Permission>(3), true); }), "combined enum value accepted as one permission");
        check(equal(before, permissions.toSaveData()), "invalid mutation changed data");
    }

    void presetsAndImmediateRevocation()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        permissions.registerUser("delegate");
        permissions.registerUser("target");
        permissions.setPermission("host", "delegate", Permission::ChangePermissions, true);
        for (const auto preset : {PermissionPreset::Master, PermissionPreset::Player, PermissionPreset::Spectator})
        {
            for (const bool delegatedTarget : {false, true})
            {
                permissions.setPermission("host", "target", Permission::ChangePermissions, delegatedTarget);
                permissions.setPermission("host", "target", Permission::Permission5, true);
                permissions.applyPreset("delegate", "target", preset);
                auto expected = permissionsForPreset(preset);
                if (delegatedTarget) expected.add(Permission::ChangePermissions);
                check(permissions.getPermissions("target") == expected, "preset merged ordinary flags or changed ChangePermissions");
                permissions.setPermission("host", "target", Permission::Permission1, false);
                check(!permissions.hasPermission("target", Permission::Permission1), "preset remained an inherited role");
            }
        }
        const auto before = permissions.toSaveData();
        check(rejects([&] { permissions.applyPreset("delegate", "delegate", PermissionPreset::Master); }), "delegate applied a preset to itself");
        check(rejects([&] { permissions.applyPreset("delegate", "host", PermissionPreset::Spectator); }), "delegate changed host through a preset");
        check(rejects([&] { permissions.applyPreset("host", "host", PermissionPreset::Spectator); }), "preset changed immutable host permissions");
        check(rejects([&] { permissions.applyPreset("host", "target", static_cast<PermissionPreset>(99)); }), "unknown preset accepted");
        check(rejects([&] { permissions.applyPreset("missing", "target", PermissionPreset::Master); }), "unknown actor applied preset");
        check(rejects([&] { permissions.applyPreset("host", "missing", PermissionPreset::Master); }), "unknown target accepted a preset");
        check(equal(before, permissions.toSaveData()), "failed preset changed data");
        permissions.setPermission("host", "delegate", Permission::ChangePermissions, false);
        check(rejects([&] { permissions.setPermission("delegate", "target", Permission::Permission2, true); }), "revoked authority remained cached");
        check(rejects([&] { permissions.applyPreset("delegate", "target", PermissionPreset::Master); }), "revoked actor could still apply a preset");
    }

    void snapshotsAndTransactionalRegistration()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        permissions.registerUser("alice");
        auto saved = permissions.toSaveData();
        check(saved.size() == 2 && saved[0].userId == "alice" && saved[1].userId == "host", "snapshot is not sorted");
        saved[0].permissions = Permission::ChangePermissions;
        check(!permissions.hasPermission("alice", Permission::ChangePermissions), "snapshot aliases live permissions");
        permissions.restoreSaveData(saved);
        check(permissions.getPermissions("alice") == PermissionSet(Permission::ChangePermissions), "restore lost a custom non-preset set");
        permissions.restoreSaveData({{"host", PermissionSet::all()}, {"alice", {}}});
        check(permissions.getPermissions("alice") == PermissionSet{}, "empty permission set cannot be restored");
        const auto before = permissions.toSaveData();
        const std::vector<std::vector<UserPermissionsSaveData>> invalid{
            {}, {{"alice", PermissionSet::all()}}, {{"other-host", PermissionSet::all()}}, {{"host", {}}},
            {{"host", permissionsForPreset(PermissionPreset::Master)}}, {{"host", PermissionSet::all()}, {"host", PermissionSet::all()}},
            {{"host", PermissionSet::all()}, {"bad id", {}}}
        };
        for (const auto &candidate : invalid)
        {
            check(rejects([&] { permissions.restoreSaveData(candidate); }), "invalid snapshot restored");
            check(equal(before, permissions.toSaveData()) && permissions.getHostId() == "host", "failed restore changed state or host identity");
        }
        int commits = 0;
        check(rejects([&]
        {
            permissions.registerUserWithCommit("bob", [&](const auto &candidate)
            {
                ++commits;
                check(candidate.size() == 3 && candidate[1].userId == "bob" && candidate[1].permissions == permissionsForPreset(PermissionPreset::Player), "wrong staged candidate");
                throw std::invalid_argument("simulated persistence failure");
            });
        }), "commit failure did not propagate");
        check(commits == 1 && equal(before, permissions.toSaveData()), "failed commit registered a user");
        permissions.registerUserWithCommit("bob", [&](const auto &candidate) { ++commits; saved = candidate; });
        check(equal(saved, permissions.toSaveData()) && commits == 2, "successful commit did not install staged state");
        check(rejects([&] { permissions.registerUserWithCommit("bob", [&](const auto &) { ++commits; }); }), "duplicate staged user accepted");
        check(rejects([&] { permissions.registerUserWithCommit("new-user", {}); }), "empty commit accepted");
        check(commits == 2 && rejects([&] { permissions.getPermissions("new-user"); }), "rejected registration changed state");
    }

    void registryIntegrationAndLifetime()
    {
        ResourceManager resources;
        resources.registerFeature<int()>("unrelated", [] { return 12; });
        {
            PermissionManager permissions(resources, "host");
            check(resources.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId") == "host", "host feature is wrong");
            resources.invoke<PermissionManager::RegisterUserSignature>("permissions.registerUser", "alice");
            resources.invoke<PermissionManager::SetPermissionSignature>("permissions.setPermission", "host", "alice", Permission::Permission5, true);
            check(resources.invoke<PermissionManager::HasPermissionSignature>("permissions.hasPermission", "alice", Permission::Permission5), "single permission feature is wrong");
            check(resources.invoke<PermissionManager::HasPermissionsSignature>("permissions.hasPermissions", "alice", Permission::Permission1 | Permission::Permission5), "combined permission feature is wrong");
            resources.invoke<PermissionManager::ApplyPresetSignature>("permissions.applyPreset", "host", "alice", PermissionPreset::Spectator);
            check(resources.invoke<PermissionManager::GetPermissionsSignature>("permissions.getPermissions", "alice") == PermissionSet(Permission::Permission1), "preset feature is wrong");
            auto saved = resources.invoke<PermissionManager::ToSaveDataSignature>("permissions.toSaveData");
            resources.invoke<PermissionManager::RegisterUserWithCommitSignature>("permissions.registerUserWithCommit", "bob", [](const auto &) {});
            resources.invoke<PermissionManager::RestoreSaveDataSignature>("permissions.restoreSaveData", saved);
            check(rejects([&] { permissions.getPermissions("bob"); }), "restore feature did not replace state");
            check(rejects([&] { PermissionManager duplicate(resources, "another-host"); }), "duplicate provider accepted");
            check(resources.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId") == "host", "collision removed original provider");
        }
        for (const auto feature : Features)
        {
            check(rejects([&] { resources.invoke<void()>(feature); }), "feature survived provider destruction");
            resources.registerFeature<void()>(feature, [] {});
            resources.unregisterFeature(feature);
        }
        for (const auto collision : Features)
        {
            resources.registerFeature<int()>(collision, [] { return 42; });
            check(rejects([&] { PermissionManager failed(resources, "host"); }), "feature collision accepted");
            check(resources.invoke<int()>(collision) == 42, "rollback removed the colliding provider");
            for (const auto other : Features)
            {
                if (other == collision) continue;
                resources.registerFeature<void()>(other, [] {});
                resources.unregisterFeature(other);
            }
            resources.unregisterFeature(collision);
        }
        check(resources.invoke<int()>("unrelated") == 12, "cleanup removed an unrelated feature");
        PermissionManager retry(resources, "retry-host");
        check(retry.getHostId() == "retry-host", "provider could not be recreated");
    }

    void transactionalPermissionChanges()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        permissions.registerUser("delegate");
        permissions.registerUser("target");
        permissions.setPermission("host", "delegate", Permission::ChangePermissions, true);
        const auto original = permissions.toSaveData();
        int commits = 0;
        const PermissionManager::PermissionsCommit failed = [&](const auto &candidate)
        {
            ++commits;
            check(candidate.size() == 3 && candidate[2].userId == "target", "permission candidate is not sorted");
            throw std::runtime_error("simulated write failure");
        };
        for (bool preset : {false, true})
        {
            bool propagated = false;
            try
            {
                if (preset) permissions.applyPresetWithCommit("delegate", "target", PermissionPreset::Master, failed);
                else permissions.setPermissionWithCommit("delegate", "target", Permission::Permission5, true, failed);
            }
            catch (const std::runtime_error &) { propagated = true; }
            check(propagated && equal(original, permissions.toSaveData()), "failed permission commit changed state");
        }
        const auto saved = [&](const auto &candidate) { ++commits; check(candidate.back().permissions.hasAll(Permission::Permission5), "candidate did not contain the requested flag"); };
        const auto result = resources.invoke<PermissionManager::SetPermissionWithCommitSignature>("permissions.setPermissionWithCommit", "delegate", "target", Permission::Permission5, true, saved);
        check(result.toMask() == 23 && permissions.getPermissions("target") == result, "committed permission result is wrong");
        permissions.setPermission("host", "target", Permission::ChangePermissions, true);
        const auto presetResult = resources.invoke<PermissionManager::ApplyPresetWithCommitSignature>("permissions.applyPresetWithCommit", "delegate", "target", PermissionPreset::Spectator, [](const auto &) {});
        check(presetResult.toMask() == 33, "transactional preset lost administrative permission");
        const int before = commits;
        check(rejects([&] { permissions.setPermissionWithCommit("delegate", "target", Permission::ChangePermissions, false, saved); }), "delegate revoked administrative permission through commit");
        check(rejects([&] { permissions.setPermissionWithCommit("host", "target", Permission::Permission1, false, {}); }), "empty permission commit accepted");
        check(rejects([&] { permissions.applyPresetWithCommit("host", "target", PermissionPreset::Master, {}); }), "empty preset commit accepted");
        check(commits == before && permissions.getPermissions("target").toMask() == 33, "denied transaction invoked its commit or changed state");
    }

    void concurrentAccessAndIndependentInstances()
    {
        ResourceManager resources;
        PermissionManager permissions(resources, "host");
        std::atomic<int> commits{0};
        std::vector<std::future<void>> workers;
        for (int index = 0; index < 8; ++index)
        {
            workers.push_back(std::async(std::launch::async, [&, index]
            {
                const auto userId = "user-" + std::to_string(index);
                resources.invoke<PermissionManager::RegisterUserWithCommitSignature>("permissions.registerUserWithCommit", userId, [&](const auto &candidate)
                {
                    check(std::is_sorted(candidate.begin(), candidate.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; }), "concurrent candidate is not sorted");
                    ++commits;
                });
                for (int iteration = 0; iteration < 50; ++iteration)
                {
                    permissions.setPermission("host", userId, Permission::Permission5, true);
                    check(permissions.hasPermission(userId, Permission::Permission5), "concurrent mutation was lost");
                    permissions.applyPreset("host", userId, PermissionPreset::Spectator);
                    check(permissions.getPermissions(userId) == PermissionSet(Permission::Permission1), "concurrent preset left mixed state");
                    const auto snapshot = permissions.toSaveData();
                    check(snapshot.front().userId == "host" && snapshot.front().permissions == PermissionSet::all(), "concurrent snapshot lost immutable host");
                }
            }));
        }
        for (auto &worker : workers) worker.get();
        check(commits == 8 && permissions.toSaveData().size() == 9, "concurrent registrations were lost");
        workers.clear();
        std::atomic<int> registered{0};
        for (int index = 0; index < 8; ++index) workers.push_back(std::async(std::launch::async, [&] { if (!rejects([&] { permissions.registerUser("same-user"); })) ++registered; }));
        for (auto &worker : workers) worker.get();
        check(registered == 1, "concurrent duplicate registration succeeded more than once");

        ResourceManager otherResources;
        PermissionManager other(otherResources, "host");
        other.registerUser("user-0");
        other.setPermission("host", "user-0", Permission::ChangePermissions, true);
        check(!permissions.hasPermission("user-0", Permission::ChangePermissions), "independent managers share state");
    }
}

int main()
{
    try
    {
        bootstrapAndDetachedChecks();
        completeMutationPolicy();
        presetsAndImmediateRevocation();
        snapshotsAndTransactionalRegistration();
        registryIntegrationAndLifetime();
        transactionalPermissionChanges();
        concurrentAccessAndIndependentInstances();
        std::cout << "PermissionManager: PASS\n";
        return 0;
    }
    catch (const std::exception &error) { std::cerr << "PermissionManager: " << error.what() << '\n'; return 1; }
}
