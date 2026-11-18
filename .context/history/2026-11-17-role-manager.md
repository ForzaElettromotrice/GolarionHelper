# Archived RoleManager API

Retired on 2026-11-17 during permission integration. This file is historical; current APIs are in ../session-permissions.md and ../protocol-manager.md. RoleManager source/tests and roles.* features have been removed.

# Session Roles

Status: legacy runtime model, implemented 2026-11-12. The user replaced the target role hierarchy with independent bitmask permissions on 2026-11-17. PermissionManager and its tests are implemented separately; see `session-permissions.md` for the current decisions. Session, ProtocolManager, client DTOs and identity archives still consume RoleManager until the next integration step. This document describes those existing legacy APIs, not the replacement authorization policy. Do not add new role RPCs.

## Ownership and Identity

- Host `Session::createHost(hostId)` (or legacy `Session(hostId)`) owns ResourceManager, ProtocolManager and an in-place optional RoleManager, passing the registry by reference. Client Sessions omit RoleManager and receive their own admitted role from the host; see `session-client.md`. No subsystem getter is exposed. Host ProtocolManager checks roles through ResourceManager at execution time.
- `UserId` is a shared alias for `std::string` in `golarion/play/user_id.hpp`, included by both managers. IDs are exact and case sensitive; registration rejects empty IDs and ASCII whitespace/control characters. They are stable identity keys, separate from IP addresses, display names, and authentication tokens.
- RoleManager starts with exactly one `Host`. Participant roles are `Spectator`, `Player`, `Master`, and `Host`. The enum also contains `Any = -1`, below Spectator, as an access threshold only: it cannot be assigned, registered, restored, or saved as a participant role. Existing participant enum values are unchanged. RoleManager cannot be copied or moved because registered callbacks refer to its address.

## API

| Operation | Behavior |
| --- | --- |
| `RoleManager(ResourceManager &resources, UserId hostId)` | Creates the immutable host and publishes the eight role features. |
| `registerUser(UserId userId)` | Creates a new `Player`; duplicate IDs are rejected without changing the existing role. Exposed to trusted local admission code through `roles.registerUser`; registry publication does not expose a network command. |
| `registerUserWithCommit(UserId userId, const RegistrationCommit &commit)` | Prepares a Player and invokes the supplied callback with the sorted candidate snapshot under the role lock. Only successful completion installs the candidate; duplicates, empty callbacks, or exceptions preserve previous roles. Used by admission to save combined identities/credentials before committing. |
| `getRole(const UserId &userId) const` | Returns the current role; unknown users are rejected. |
| `getHostId() const` | Returns the unique immutable Host ID under the role mutex. Used through ResourceManager by local sendCommand; never derived from a payload. |
| `hasAtLeastRole(const std::optional<UserId> &userId, Role minimumRole) const` | Checks `Host > Master > Player > Spectator > Any`. `nullopt` satisfies only Any; a supplied unknown/empty UserId and invalid enum values are rejected, including for minimum Any. |
| `setRole(const UserId &actorId, const UserId &targetId, Role newRole)` | Checks both current roles and the requested role before mutation, including host immutability, host-only changes to masters, and rejection of Any. All denied operations preserve the previous state. |
| `toSaveData() const` | Returns detached `UserRoleSaveData` records sorted by UserId. |
| `restoreSaveData(const std::vector<UserRoleSaveData> &)` | Validates the full candidate before replacement: valid unique IDs and participant roles (never Any), exactly one Host, and the same immutable Host UserId. Trusted local restoration only. |

Invalid IDs, duplicate users, unknown users, invalid enum values, and unauthorized role changes throw `std::invalid_argument`. Permission checks precede any no-op treatment: a forbidden request remains forbidden even when the requested role equals the current one. There is no disconnect/removal operation in this component. Session delegates `registerUser`; archive save/load delegates to ProtocolManager, which obtains/restores roles through the registry. Role changes after a save require another explicit save.

## ResourceManager Features

| Feature | Signature alias on RoleManager |
| --- | --- |
| `roles.registerUser` | `RegisterUserSignature = void(UserId)` |
| `roles.getRole` | `GetRoleSignature = Role(const UserId &)` |
| `roles.getHostId` | `GetHostIdSignature = UserId()` |
| `roles.hasAtLeastRole` | `HasAtLeastRoleSignature = bool(const std::optional<UserId> &, Role)` |
| `roles.setRole` | `SetRoleSignature = void(const UserId &, const UserId &, Role)` |
| `roles.toSaveData` | `ToSaveDataSignature = std::vector<UserRoleSaveData>()` |
| `roles.restoreSaveData` | `RestoreSaveDataSignature = void(const std::vector<UserRoleSaveData> &)` |
| `roles.registerUserWithCommit` | `RegisterUserWithCommitSignature = void(UserId, RegistrationCommit)`; `RegistrationCommit = std::function<void(const std::vector<UserRoleSaveData> &)>` |

```cpp
resources.invoke<RoleManager::RegisterUserSignature>("roles.registerUser", userId);
const Role role = resources.invoke<RoleManager::GetRoleSignature>("roles.getRole", userId);
resources.invoke<RoleManager::SetRoleSignature>("roles.setRole", actorId, targetId, Role::Spectator);
```

RoleManager owns these eight feature names for its lifetime; other subsystems must not remove or replace them. Destruction unregisters them before the referenced object is destroyed. Constructor collision rollback preserves pre-existing features. ResourceManager must outlive RoleManager. Complete data operations use an internal mutex; register providers before work starts and stop both networking and local command workers before removing them. The staged registration callback must not reenter RoleManager or acquire ProtocolManager locks. Snapshot restoration and commit callbacks are trusted local functionality and must not become remote commands.

Registry calls perform the same domain validation as direct methods. ProtocolManager derives optional identity from live connection records and checks minimum roles through the registry, also for Any commands. RoleManager does not decide whether a connection is live. Concrete role-command adapters remain future work; their minimum role must require authentication and they use `*context.userId` as actor. Payload actor IDs are not trusted. RoleManager itself does not authenticate sockets or host a server; see `session-commands.md`.

## Verification

`play_role_manager_test` covers bootstrap/default roles, immutable host lookup, duplicate and invalid IDs, the complete 64-case participant permission matrix, self-change restrictions, invalid enums, all 25 anonymous/participant minimum-role comparisons, rejection of Any assignments/restores, registry dispatch/demotion, destruction/constructor rollback for all eight feature-name collisions, independent registries, and Session construction. It also covers sorted/detached snapshots, invalid restores, staged-registration rollback/commit, and concurrent registration/changes/snapshots. All thirteen play suites pass through CMake/CTest on Windows GCC C++20.
