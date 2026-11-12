#include "golarion/play/role_manager.hpp"
#include "golarion/play/session.hpp"

#include <array>
#include <algorithm>
#include <cassert>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    using namespace golarion::play;

    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void bootstrapAndRegistration()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        assert(roles.getRole("host") == Role::Host);
        assert(rejects([&] { roles.registerUser("host"); }));
        roles.registerUser("alice");
        assert(roles.getRole("alice") == Role::Player);
        roles.setRole("host", "alice", Role::Master);
        assert(rejects([&] { roles.registerUser("alice"); }));
        assert(roles.getRole("alice") == Role::Master);
        roles.registerUser("Alice");
        assert(roles.getRole("Alice") == Role::Player);

        for (const UserId invalidId : {"", " ", "bad id", "bad\tid", "bad\nid", "bad\x7f"})
        {
            assert(rejects([&] { roles.registerUser(invalidId); }));
            ResourceManager empty;
            assert(rejects([&] { RoleManager invalid(empty, invalidId); }));
            assert(rejects([&] { empty.invoke<RoleManager::GetRoleSignature>("roles.getRole", "host"); }));
        }
        assert(rejects([&] { roles.getRole("missing"); }));
        assert(rejects([&] { roles.hasAtLeastRole("missing", Role::Spectator); }));
        assert(rejects([&] { roles.setRole("missing", "alice", Role::Player); }));
        assert(rejects([&] { roles.setRole("host", "missing", Role::Player); }));
        assert(roles.getRole("alice") == Role::Master);
    }

    void completePermissionMatrix()
    {
        constexpr std::array<Role, 4> allRoles{Role::Spectator, Role::Player, Role::Master, Role::Host};
        // Rows are actor roles; columns are requested roles for the given target.
        constexpr bool playerTarget[4][4] = {{false, false, false, false}, {false, false, false, false}, {true, true, false, false}, {true, true, true, false}};
        constexpr bool spectatorTarget[4][4] = {{false, false, false, false}, {false, false, false, false}, {true, true, false, false}, {true, true, true, false}};
        constexpr bool masterTarget[4][4] = {{false, false, false, false}, {false, false, false, false}, {false, false, false, false}, {true, true, true, false}};
        constexpr bool hostTarget[4][4] = {};
        const bool (*permissions[4])[4] = {spectatorTarget, playerTarget, masterTarget, hostTarget};

        for (std::size_t actorIndex = 0; actorIndex < allRoles.size(); ++actorIndex)
        {
            for (std::size_t targetIndex = 0; targetIndex < allRoles.size(); ++targetIndex)
            {
                for (std::size_t requestedIndex = 0; requestedIndex < allRoles.size(); ++requestedIndex)
                {
                    ResourceManager resources;
                    RoleManager roles(resources, "host");
                    roles.registerUser("actor");
                    roles.registerUser("target");
                    const UserId actorId = allRoles[actorIndex] == Role::Host ? "host" : "actor";
                    const UserId targetId = allRoles[targetIndex] == Role::Host ? "host" : "target";
                    if (actorId != "host") roles.setRole("host", actorId, allRoles[actorIndex]);
                    if (targetId != "host") roles.setRole("host", targetId, allRoles[targetIndex]);

                    const bool accepted = !rejects([&] { roles.setRole(actorId, targetId, allRoles[requestedIndex]); });
                    assert(accepted == permissions[targetIndex][actorIndex][requestedIndex]);
                    assert(roles.getRole(targetId) == (accepted ? allRoles[requestedIndex] : allRoles[targetIndex]));
                    assert(roles.getRole("host") == Role::Host);
                }
            }
        }
    }

    void selfChangesAndInvalidRoles()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        roles.registerUser("user");
        constexpr std::array<Role, 3> ordinaryRoles{Role::Spectator, Role::Player, Role::Master};
        constexpr std::array<Role, 4> allRoles{Role::Spectator, Role::Player, Role::Master, Role::Host};
        for (const Role current : ordinaryRoles)
        {
            roles.setRole("host", "user", current);
            for (const Role requested : allRoles)
            {
                assert(rejects([&] { roles.setRole("user", "user", requested); }));
                assert(roles.getRole("user") == current);
            }
        }
        for (const Role requested : allRoles)
        {
            assert(rejects([&] { roles.setRole("host", "host", requested); }));
            assert(roles.getRole("host") == Role::Host);
        }
        for (const Role invalid : {static_cast<Role>(-2), static_cast<Role>(4), static_cast<Role>(99)})
        {
            assert(rejects([&] { roles.setRole("host", "user", invalid); }));
            assert(rejects([&] { roles.hasAtLeastRole("host", invalid); }));
            assert(roles.getRole("user") == Role::Master);
        }
    }

    void minimumRoleAndRegistryDispatch()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        const std::array<UserId, 4> users{"spectator", "player", "master", "host"};
        constexpr std::array<Role, 4> allRoles{Role::Spectator, Role::Player, Role::Master, Role::Host};
        constexpr bool allowed[4][4] = {{true, false, false, false}, {true, true, false, false}, {true, true, true, false}, {true, true, true, true}};
        for (std::size_t index = 0; index < 3; ++index)
        {
            resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", users[index]);
            assert(roles.getRole(users[index]) == Role::Player);
            resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", "host", users[index], allRoles[index]);
        }
        for (std::size_t userIndex = 0; userIndex < users.size(); ++userIndex)
        {
            assert(resources.invoke<RoleManager::GetRoleSignature>("roles.getRole", users[userIndex]) == allRoles[userIndex]);
            for (std::size_t minimumIndex = 0; minimumIndex < allRoles.size(); ++minimumIndex)
            {
                assert(roles.hasAtLeastRole(users[userIndex], allRoles[minimumIndex]) == allowed[userIndex][minimumIndex]);
                assert(resources.invoke<RoleManager::HasAtLeastRoleSignature>("roles.hasAtLeastRole", users[userIndex], allRoles[minimumIndex]) == allowed[userIndex][minimumIndex]);
            }
        }
        assert(rejects([&] { resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", "host"); }));
        assert(rejects([&] { resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", "master"); }));
        assert(roles.getRole("master") == Role::Master);
        assert(roles.getRole("host") == Role::Host);
        for (const UserId invalidId : {"", " ", "bad id", "bad\nid"})
        {
            assert(rejects([&] { resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", invalidId); }));
            assert(rejects([&] { roles.getRole(invalidId); }));
        }
        assert(rejects([&] { resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", "master", "player", Role::Master); }));
        assert(rejects([&] { resources.invoke<RoleManager::GetRoleSignature>("roles.getRole", "missing"); }));
        assert(rejects([&] { resources.invoke<RoleManager::HasAtLeastRoleSignature>("roles.hasAtLeastRole", "missing", Role::Spectator); }));

        resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", "host", "master", Role::Spectator);
        assert(!resources.invoke<RoleManager::HasAtLeastRoleSignature>("roles.hasAtLeastRole", "master", Role::Master));
        assert(rejects([&] { resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", "master", "player", Role::Spectator); }));
        assert(roles.getRole("player") == Role::Player);
    }

    void anyIsOnlyAnAccessThreshold()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        const std::array<std::optional<UserId>, 5> users{std::nullopt, "spectator", "player", "master", "host"};
        constexpr std::array<Role, 5> levels{Role::Any, Role::Spectator, Role::Player, Role::Master, Role::Host};
        constexpr bool allowed[5][5]{
            {true, false, false, false, false}, {true, true, false, false, false},
            {true, true, true, false, false}, {true, true, true, true, false}, {true, true, true, true, true}
        };
        for (std::size_t index = 1; index < 4; ++index)
        {
            roles.registerUser(*users[index]);
            roles.setRole("host", *users[index], levels[index]);
        }
        for (std::size_t user = 0; user < users.size(); ++user)
        {
            for (std::size_t minimum = 0; minimum < levels.size(); ++minimum)
            {
                assert(roles.hasAtLeastRole(users[user], levels[minimum]) == allowed[user][minimum]);
                assert(resources.invoke<RoleManager::HasAtLeastRoleSignature>("roles.hasAtLeastRole", users[user], levels[minimum]) == allowed[user][minimum]);
            }
        }
        assert(rejects([&] { roles.hasAtLeastRole("missing", Role::Any); }));
        assert(rejects([&] { roles.hasAtLeastRole(UserId{}, Role::Any); }));
        assert(rejects([&] { roles.hasAtLeastRole(std::nullopt, static_cast<Role>(99)); }));
        for (std::size_t actor = 1; actor < users.size(); ++actor)
        {
            for (std::size_t target = 1; target < users.size(); ++target)
            {
                assert(rejects([&] { roles.setRole(*users[actor], *users[target], Role::Any); }));
                assert(roles.getRole(*users[target]) == levels[target]);
            }
        }
        assert(rejects([&] { roles.restoreSaveData({{"host", Role::Host}, {"anonymous", Role::Any}}); }));
        assert(roles.toSaveData().size() == 4 && roles.getRole("master") == Role::Master);
    }

    void registryLifetimeAndConstructionRollback()
    {
        constexpr std::array<std::string_view, 7> features{"roles.getRole", "roles.hasAtLeastRole", "roles.setRole", "roles.registerUser", "roles.toSaveData", "roles.restoreSaveData", "roles.registerUserWithCommit"};
        ResourceManager resources;
        resources.registerFeature<int()>("unrelated", [] { return 9; });
        {
            RoleManager roles(resources, "host");
            assert(rejects([&] { RoleManager duplicate(resources, "other-host"); }));
            assert(resources.invoke<RoleManager::GetRoleSignature>("roles.getRole", "host") == Role::Host);
        }
        for (const auto name : features)
        {
            assert(rejects([&] { resources.invoke<void()>(name); }));
            resources.registerFeature<void()>(name, [] {});
            resources.unregisterFeature(name);
        }
        assert(resources.invoke<int()>("unrelated") == 9);

        for (const auto collision : features)
        {
            resources.registerFeature<int()>(collision, [] { return 42; });
            assert(rejects([&] { RoleManager failed(resources, "host"); }));
            assert(resources.invoke<int()>(collision) == 42);
            for (const auto other : features)
            {
                if (other == collision) continue;
                resources.registerFeature<void()>(other, [] {});
                resources.unregisterFeature(other);
            }
            resources.unregisterFeature(collision);
        }
        RoleManager retry(resources, "retry-host");
        assert(retry.getRole("retry-host") == Role::Host);
    }

    void savedRolesAndInvalidRestoration()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        roles.registerUser("alice");
        roles.setRole("host", "alice", Role::Master);
        const auto saved = resources.invoke<RoleManager::ToSaveDataSignature>("roles.toSaveData");
        assert(saved.size() == 2 && saved[0].userId == "alice" && saved[0].role == Role::Master);
        roles.setRole("host", "alice", Role::Spectator);
        resources.invoke<RoleManager::RestoreSaveDataSignature>("roles.restoreSaveData", saved);
        assert(roles.getRole("alice") == Role::Master && roles.getRole("host") == Role::Host);
        const std::vector<std::vector<UserRoleSaveData>> invalid{
            {}, {{"alice", Role::Player}}, {{"host", Role::Player}},
            {{"host", Role::Host}, {"other", Role::Host}}, {{"other", Role::Host}},
            {{"host", Role::Host}, {"host", Role::Player}},
            {{"host", Role::Host}, {"alice", static_cast<Role>(99)}},
            {{"host", Role::Host}, {"bad id", Role::Player}}
        };
        for (const auto &data : invalid)
        {
            assert(rejects([&] { roles.restoreSaveData(data); }));
            assert(roles.getRole("alice") == Role::Master && roles.getRole("host") == Role::Host);
        }
    }

    void stagedRegistrationAndConcurrentAccess()
    {
        ResourceManager resources;
        RoleManager roles(resources, "host");
        roles.registerUser("alice");
        roles.setRole("host", "alice", Role::Master);
        bool called = false;
        assert(rejects([&]
        {
            roles.registerUserWithCommit("candidate", [&](const auto &data)
            {
                called = true;
                assert(data.size() == 3 && data[0].userId == "alice" && data[1].userId == "candidate" && data[1].role == Role::Player);
                throw std::invalid_argument("simulated persistence failure");
            });
        }));
        assert(called && rejects([&] { roles.getRole("candidate"); }));
        assert(roles.getRole("alice") == Role::Master && roles.toSaveData().size() == 2);
        called = false;
        assert(rejects([&] { roles.registerUserWithCommit("alice", [&](const auto &) { called = true; }); }));
        assert(!called);
        assert(rejects([&] { roles.registerUserWithCommit("bad id", [&](const auto &) { called = true; }); }));
        assert(!called);
        assert(rejects([&] { roles.registerUserWithCommit("candidate", {}); }));
        resources.invoke<RoleManager::RegisterUserWithCommitSignature>("roles.registerUserWithCommit", "candidate", [&](const auto &data)
        {
            called = true;
            assert(data.size() == 3);
        });
        assert(called && roles.getRole("candidate") == Role::Player);

        std::vector<std::future<void>> tasks;
        for (int index = 0; index < 4; ++index)
        {
            tasks.push_back(std::async(std::launch::async, [&, index]
            {
                for (int iteration = 0; iteration < 25; ++iteration)
                {
                    const auto userId = "concurrent-" + std::to_string(index) + "-" + std::to_string(iteration);
                    roles.registerUserWithCommit(userId, [&](const auto &data)
                    {
                        assert(std::any_of(data.begin(), data.end(), [&](const auto &entry) { return entry.userId == userId && entry.role == Role::Player; }));
                    });
                    roles.setRole("host", userId, Role::Master);
                    assert(roles.getRole(userId) == Role::Master && roles.hasAtLeastRole(userId, Role::Player));
                    const auto saved = roles.toSaveData();
                    assert(std::is_sorted(saved.begin(), saved.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; }));
                    assert(roles.getRole("host") == Role::Host);
                }
            }));
        }
        for (auto &task : tasks) task.get();
        assert(roles.toSaveData().size() == 103);
    }

    void independentInstancesAndSessionConstruction()
    {
        ResourceManager firstResources;
        ResourceManager secondResources;
        RoleManager first(firstResources, "host");
        RoleManager second(secondResources, "host");
        first.registerUser("user");
        second.registerUser("user");
        first.setRole("host", "user", Role::Master);
        assert(second.getRole("user") == Role::Player);
        assert(first.getRole("user") == Role::Master);

        assert(rejects([] { Session session(""); }));
        Session session("host");
        Session other("host");
    }
}

int main()
{
    bootstrapAndRegistration();
    completePermissionMatrix();
    selfChangesAndInvalidRoles();
    minimumRoleAndRegistryDispatch();
    anyIsOnlyAnAccessThreshold();
    registryLifetimeAndConstructionRollback();
    savedRolesAndInvalidRestoration();
    stagedRegistrationAndConcurrentAccess();
    independentInstancesAndSessionConstruction();
}
