# Permission Commands

Status: transactional RPCs implemented 2026-11-17, targeted live notifications added 2026-11-18. The desktop permission editor uses an atomic update RPC. Existing JSON v4, sendCommand and common dispatch are reused. Effective successful mutations emit permissions.changed only to the affected user; see session-events.md.

## Registration and API

PermissionManager::registerCommands() in server/src/permission_commands.cpp registers callbacks through ResourceManager's protocol.add_command. Host Session calls it after constructing its managers, before networking starts. A standalone PermissionManager still works without ServerProtocol until this explicit method is called. Register once during setup; duplicates reject without replacing callbacks. Session remains a facade.

Callbacks capture ResourceManager and call typed permission features. Actor identity comes only from CommandContext, verified by dispatch. Payload selects the target and change, never the actor. Providers outlive workers; fixed registration/removal is not concurrent with invocation. Direct ServerProtocol users call registerCommands explicitly if they want these built-ins.

| Command | Exact payload | Access |
| --- | --- | --- |
| permissions.get | Empty object for self, or only targetUserId | Authenticated zero-mask requirement. Reading another user requires ChangePermissions; self-read works with mask zero. |
| permissions.applyPreset | targetUserId, preset | ChangePermissions plus the same restrictions. |
| permissions.update | targetUserId, grant, revoke | ChangePermissions plus the same restrictions, including host-only delegation. |

Success returns exactly userId and the effective numeric permissions inside the existing payload.result wrapper. No preset label or credential is returned. Reads query current server state without archive I/O. ClientSnapshot refreshes through the built-in permissions.changed notification handler, independently of the command response.

The target must be a valid, known, case-sensitive UserId. Extra fields including actorId reject. Presets accept only master, player and spectator, converted to PermissionPreset at the input boundary.

For update, grant and revoke are disjoint validated masks. Each may be zero. The server applies both masks to the current state under the same lock, persists once and returns the resulting mask. Untouched flags retain their current values even if another actor changed them since the editor opened.

```cpp
auto reply = session.sendCommand("permissions.update", {
    {"targetUserId", targetId}, {"grant", 16}, {"revoke", 0}
});
const auto result = reply.get(); // Player mask 7 becomes 23.
```

```json
{"version":4,"type":"permissions.update","requestId":"client-4","payload":{"targetUserId":"user-example","grant":16,"revoke":0}}
{"version":4,"type":"request.accepted","requestId":"client-4","payload":{"result":{"userId":"user-example","permissions":23}}}
```

Only Host grants/revokes ChangePermissions. Delegates change other non-host users, including other delegates, but cannot target self or Host, including no-ops. Presets replace ordinary flags and preserve the target's ChangePermissions bit. Full mask 63 does not grant Host identity. Current permissions are checked on every execution regardless of a client's cached admission mask.

## Transaction Coordination

Mutations require a previously configured saveIdentityState/loadIdentityState archive, also for offline host commands. Without it they fail with COMMAND_FAILED without changing memory. Local and remote callers share the callback/execution path.

ServerProtocol publishes trusted internal identity.updateUserPermissions, defined by private identity_transaction.hpp. It accepts a synchronous UserPermissionsMutation and returns PermissionSet. It acquires execution then identity locks, snapshots Host ID and credentials and supplies one persistence callback. The callback must be used synchronously once, must not escape, and writes the existing version-4 archive without reentering managers or their locks. It is not a network command.

PermissionManager's transactional mutation features validate restrictions under its mutex, build a candidate map/sorted snapshot, invoke persistence, then install the candidate with a nonthrowing swap and return its mask. Failed persistence leaves live state unchanged. The existing exclusive temporary file/flush/atomic replacement preserves the old file on storage failure. Lock order is execution, identity, permissions; a permission commit callback must not call saveIdentityState.

Direct updatePermissions/applyPreset remain trusted in-memory domain operations. The commands use transactional variants and save the full current permission snapshot with existing tokens before success. Tokens are not reissued/rotated. Reads do not save. No-op mutations retain authorization checks and the same commit path. Snapshot restoration/staged commit remain internal trusted APIs.

After the transaction returns, changePermissions in the command adapters compares the previous/effective masks and invokes protocol.sendEvent for that target only. No-ops, denied changes and failed writes emit nothing. Delivery cannot roll back an already committed transaction; offline targets obtain current masks on resume. Direct manager setters do not publish events.

ServerProtocol owns the identity and sendEvent features only in Host mode. Constructor collisions roll back successfully owned providers; destruction joins workers before unregistering services. Client mode owns only its callback-only protocol.add_command provider. PermissionManager manages the lifetime of its own features.

## Errors and Verification

PermissionDenied derives from std::invalid_argument for domain compatibility. Adapters translate it to FORBIDDEN. Invalid fields/flags/presets/targets yield INVALID_REQUEST; absent archives and write failures yield COMMAND_FAILED. Existing wire messages remain sanitized Italian. Ordinary admitted errors keep the connection usable.

play_permission_commands_test covers local/offline/TCP operations, service/command collisions, missing archives, strict payloads, pre-admission rejection, zero-mask reads, metadata visibility, self/host protection, host-only delegation, preset preservation, immediate revocation, concurrent local/remote transactions and full host reconstruction/resume. Batch cases verify disjoint edits are preserved across concurrent updates and a prohibited flag rejects the entire change. A blocked archive destination verifies unchanged memory and byte-identical original file contents after storage failure. Manager tests also cover failed/empty commits, candidate snapshots and feature collisions.

The repository-root Debug/Ninja CMake build passes all seventeen play suites on Windows, with binaries/tests under build/core/play. play_events_test adds targeted delivery, snapshot refresh and absence of events after failed/no-op mutations. A mutation CLI prompt remains deferred; callers use Session::sendCommand. See development.md.
