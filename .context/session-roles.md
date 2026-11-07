# Session Roles

Status: C++20 model implemented and tested, 2026-11-07. The permission policy confirmed in [game-management.md](game-management.md#confirmed-structural-decisions-for-later-steps) is enforced by RoleManager. Transport integration, identity/token handling, and persistence remain separate steps.

## Ownership and Identity

- `Session(UserId hostId)` owns its ResourceManager first and RoleManager second, passing the registry by reference. Session requires an explicit host ID and exposes no subsystem getters yet.
- `UserId` is currently an alias for `std::string` in `golarion/play/role_manager.hpp`. IDs are exact and case sensitive; registration rejects empty IDs and ASCII whitespace/control characters. They are stable identity keys, separate from IP addresses, display names, and authentication tokens.
- RoleManager starts with exactly one `Host`. The `Role` enum contains `Spectator`, `Player`, `Master`, and `Host`. RoleManager cannot be copied or moved because registered callbacks refer to its address.

## API

| Operation | Behavior |
| --- | --- |
| `RoleManager(ResourceManager &resources, UserId hostId)` | Creates the immutable host and publishes the four role features. |
| `registerUser(UserId userId)` | Creates a new `Player`; duplicate IDs are rejected without changing the existing role. Exposed to trusted local admission code through `roles.registerUser`; registry publication does not expose a network command. |
| `getRole(const UserId &userId) const` | Returns the current role; unknown users are rejected. |
| `hasAtLeastRole(const UserId &userId, Role minimumRole) const` | Checks `Host > Master > Player > Spectator`; unknown users and invalid enum values are rejected. |
| `setRole(const UserId &actorId, const UserId &targetId, Role newRole)` | Checks both current roles and the requested role before mutation, including host immutability and host-only changes to masters. All denied operations preserve the previous state. |

Invalid IDs, duplicate users, unknown users, invalid enum values, and unauthorized role changes throw `std::invalid_argument`. Permission checks precede any no-op treatment: a forbidden request remains forbidden even when the requested role equals the current one. There is no disconnect/removal operation in this component; roles remain in memory for its lifetime. Restart persistence is not implemented.

## ResourceManager Features

| Feature | Signature alias on RoleManager |
| --- | --- |
| `roles.registerUser` | `RegisterUserSignature = void(UserId)` |
| `roles.getRole` | `GetRoleSignature = Role(const UserId &)` |
| `roles.hasAtLeastRole` | `HasAtLeastRoleSignature = bool(const UserId &, Role)` |
| `roles.setRole` | `SetRoleSignature = void(const UserId &, const UserId &, Role)` |

```cpp
resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", userId);
const Role role = resources.invoke<RoleManager::GetRoleSignature>("roles.getRole", userId);
resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", actorId, targetId, Role::Spectator);
```

RoleManager owns these four feature names for its lifetime; other subsystems must not remove or replace them. Destruction unregisters them before the referenced object is destroyed. If construction encounters a name collision, it removes only its own successful registrations and preserves the pre-existing feature. The ResourceManager must outlive RoleManager. Calls run on the same serialized execution thread.

Registry dispatch performs the same domain validation as direct method calls. The future protocol must derive `actorId` from the authenticated connection; a client-supplied actor ID is not trusted. This model does not authenticate identities or start a server.

## Verification

`play_role_manager_test` covers bootstrap/default roles, duplicate and invalid IDs (also through `roles.registerUser`), the complete 64-case actor/target/requested-role permission matrix, self-change restrictions, invalid enum values, all 16 minimum-role comparisons, registry dispatch and immediate demotion, destruction/constructor rollback for all four feature-name collisions, independent registries, and Session construction. The test and the existing ResourceManager test pass with native GCC C++20 and warnings enabled. CMake remains unavailable locally.
