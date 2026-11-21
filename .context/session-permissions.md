# Session Permissions

Status: integrated 2026-11-17; targeted live refresh added 2026-11-18. Host Session owns PermissionManager; ProtocolManager checks current masks through ResourceManager, and JSON v4/client DTOs/version-3 archives use numeric masks. RoleManager is removed. Permission-command adapters notify only the affected user after effective committed changes. Connected-user snapshots/events now use authenticated zero-bit access without exposing other users' masks; see session-participants.md, session-permission-commands.md and session-events.md.

## Model and Policy

- `Permission` is a closed `enum class` with explicit `std::uint64_t` values: Any = 0 is a command-only requirement, Permission1 through Permission5 occupy bits 0 through 4, and ChangePermissions occupies bit 5. The desktop editor uses the provisional Permission1 through Permission5 labels and displays ChangePermissions as Gestire i permessi. Placeholder flags have no sheet-operation semantics yet.
- `PermissionSet` in `golarion/play/permission_set.hpp` is a value wrapper holding one private 64-bit mask. Its default value is empty. Construction accepts a single assignable typed flag or an initializer list; Any, invalid enum values and combined enum values are rejected. Combine known flags with `|`. fromMask(uint64_t) validates that only known bits are set; toMask() is used by the actual JSON/storage consumers. Numeric zero is a valid empty set, not an assigned Any flag.
- `hasAll(required)` tests `(owned & required) == required`; `hasAny(candidates)` tests a nonzero intersection. `add` and `remove` accept sets, with a validated implicit conversion from a single Permission. `all()` contains the six known flags. Sets support value equality and are trivially copyable.
- Empty requirements match every registered user; an unknown UserId still rejects. CommandRequirements(Permission::Any) explicitly selects Public access, used for join/resume. A default requirement or PermissionSet{} selects Authenticated access with no flags. Both masks are zero but have distinct access modes. Public access cannot combine participant flags, and unknown access enum values reject registration. See `session-commands.md`.
- The immutable Host UserId is stored separately from permissions. The host always owns all six permissions; runtime mutations and presets cannot target the host, including no-op changes. There is no Host role or hierarchy inside this model.
- Only the host grants or revokes ChangePermissions. A non-host possessing it can change ordinary flags on other registered participants, including another delegate, but cannot modify itself or the host. Checks run before mutation and before no-op treatment; revocations take effect on the next call.
- Master, Player and Spectator are enum-based presets, not participant roles or inherited permissions. Applying a preset replaces all ordinary flags and preserves the target's existing ChangePermissions bit. No preset grants that bit. After application individual flags may be freely customized by an authorized actor.

| Preset | Ordinary permissions |
| --- | --- |
| Spectator | Permission1 |
| Player | Permission1, Permission2, Permission3 |
| Master | Permission1, Permission2, Permission3, Permission4, Permission5 |

New registered users receive Player permissions, with no administrative flag. UserId is shared in `golarion/play/user_id.hpp`; IDs remain case sensitive and reject empty values and ASCII whitespace/control characters. PermissionManager has no dependency on RoleManager or the network/frontend.

## API and ResourceManager Features

PermissionManager receives ResourceManager by reference and Host UserId. It cannot be copied/moved because feature callbacks refer to its address. Host Session calls registerCommands() after construction to register adapters via protocol.add_command. Internal methods are exposed through ResourceManager with signature aliases on PermissionManager.

| Feature / method | Signature alias |
| --- | --- |
| `permissions.registerUser` / `registerUser` | `RegisterUserSignature = void(UserId)` |
| `permissions.getHostId` / `getHostId` | `GetHostIdSignature = UserId()` |
| `permissions.getPermissions` / `getPermissions` | `GetPermissionsSignature = PermissionSet(const UserId &)` |
| `permissions.hasPermission` / `hasPermission` | `HasPermissionSignature = bool(const UserId &, Permission)` |
| `permissions.hasPermissions` / `hasPermissions` | `HasPermissionsSignature = bool(const UserId &, PermissionSet)`; requires all flags |
| `permissions.setPermission` / `setPermission` | `SetPermissionSignature = void(const UserId &, const UserId &, Permission, bool)`; actor, target, flag, enabled |
| `permissions.applyPreset` / `applyPreset` | `ApplyPresetSignature = void(const UserId &, const UserId &, PermissionPreset)` |
| `permissions.toSaveData` / `toSaveData` | `ToSaveDataSignature = std::vector<UserPermissionsSaveData>()` |
| `permissions.restoreSaveData` / `restoreSaveData` | `RestoreSaveDataSignature = void(const std::vector<UserPermissionsSaveData> &)` |
| `permissions.registerUserWithCommit` / `registerUserWithCommit` | `RegisterUserWithCommitSignature = void(UserId, RegistrationCommit, std::string)` |
| `permissions.setPermissionWithCommit` / `setPermissionWithCommit` | `SetPermissionWithCommitSignature = PermissionSet(const UserId &, const UserId &, Permission, bool, PermissionsCommit)` |
| `permissions.applyPresetWithCommit` / `applyPresetWithCommit` | `ApplyPresetWithCommitSignature = PermissionSet(const UserId &, const UserId &, PermissionPreset, PermissionsCommit)` |
| `permissions.getDisplayName` | `GetDisplayNameSignature = std::string(const UserId &)` |
| `permissions.setDisplayNameWithCommit` | `SetDisplayNameWithCommitSignature = bool(const UserId &, std::string, RegistrationCommit)` |
| `permissions.updatePermissionsWithCommit` | `UpdatePermissionsWithCommitSignature = PermissionSet(const UserId &, const UserId &, PermissionSet, PermissionSet, PermissionsCommit)`; actor, target, grants, revocations, commit |

```cpp
ResourceManager resources;
PermissionManager permissions(resources, "host");
resources.invoke<PermissionManager::RegisterUserSignature>("permissions.registerUser", "alice");
resources.invoke<PermissionManager::SetPermissionSignature>("permissions.setPermission", "host", "alice", Permission::Permission5, true);
const auto required = Permission::Permission1 | Permission::Permission5;
const bool allowed = resources.invoke<PermissionManager::HasPermissionsSignature>("permissions.hasPermissions", "alice", required);
```

Invalid IDs, duplicate users and unknown enum values throw std::invalid_argument. Denied mutations throw PermissionDenied, derived from std::invalid_argument; adapters map it to FORBIDDEN. Returned data is detached. State operations use the manager mutex; Host ID is immutable. ResourceManager outlives the manager and fixed registration/removal is not concurrent with invocation. PermissionManager owns its feature registrations, rolls them back on collision and unregisters them on destruction.

## Snapshots and Staged Registration

`UserPermissionsSaveData` contains UserId, PermissionSet and displayName. `toSaveData` returns entries sorted by UserId. Trusted `restoreSaveData` validates names and unique IDs before atomic replacement and requires the original host with all known permissions.

`registerUserWithCommit` stages Player permissions and invokes a nonempty callback with the sorted candidate under the manager lock. Only callback success installs the candidate; failure or duplicate registration preserves current state. Admission uses this callback to write the archive. Commit callbacks must not reenter managers or acquire their locks. Snapshot restoration and staged commit are trusted local features, not network commands.

Archive v3 stores hostId, per-user masks and names, and token hashes. Loading v2 retains IDs, masks and credentials and initializes unset names. Role-based archive v1 remains unsupported. See `session-tokens.md`.

## Verification and Next Steps

The 2026-11-18 migration audit found no remaining RoleManager types, includes, build targets or roles.* services in active code. Test identifiers and diagnostics now use permission terminology. Retired role references remain only in historical documentation, migration explanations and negative tests that reject legacy commands/archives.

`play_permission_set_test` checks constexpr/value semantics, composition/removal, empty requirements, all 4096 pairs of the 64 flag combinations against independent set membership, rejected flag values and exact presets. `play_permission_manager_test` checks registration, detached state, the 4608-case delegate mutation matrix, host-only administrative changes, protected host/self targets, preset replacement with preserved delegation, immediate revocation, valid/invalid snapshots, staged-commit rollback, feature collisions/lifetime cleanup, independent registries and concurrent calls/duplicate registration.


Targeted permission synchronization is implemented through permissions.changed and the shared client registry/dispatch. ClientSnapshot refreshes on validated notifications; permissions.get and authorization read current server state. The desktop dialog uses the atomic update command and refreshes row availability when delegation changes. Full game persistence and concrete sheet permission meanings remain deferred.

The permission commands are explicitly registered. The mutations use staged-commit variants so write failure preserves memory and the original archive. ProtocolManager coordinates identity.updateUserPermissions with execution-then-identity-then-permission locking; the commit callback does not reenter managers. Actor comes from CommandContext. Direct setters remain in-memory domain operations; use command adapters for automatic persistence. Details are in session-permission-commands.md.

The repository-root CMakeLists.txt was configured with -S . -B build, Debug and Ninja. Fifteen library sources, both tools and seventeen native play suites compile under build/core/play; all seventeen suites pass through CTest on Windows. This includes RPC/transaction coverage, live notifications and existing streaming, identity and buffered-stop regressions. See development.md for exact commands; earlier standalone verification is historical.
