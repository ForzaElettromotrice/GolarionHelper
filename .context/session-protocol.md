# Session Protocol

Status: JSON v3 admission/commands and targeted permissions.changed notifications implemented, 2026-11-18. Permission RPCs save transactionally and refresh the affected client. User presence events, sheet commands, display names, full game persistence and frontend remain separate work.

## Session Code and Connections

SessionCode has exactly five ASCII A-Z/0-9 symbols, accepts lowercase by normalization and is generated once for a host ProtocolManager. It remains stable across that live Session's hosting restarts. Host-wide uniqueness/collision registry and persistence are deferred; a reconstructed Session generates another candidate. Clients need reachable IPv4, port and current code; resume additionally needs the original token.

ProtocolManager owns logical ConnectionIds, pending records and active UserId bindings. registerConnection allocates nonzero monotonically increasing IDs. bindUser associates an identity verified by its caller and replaces any previous binding. makeContext checks existence/both directions; closed/replaced peers are not anonymous peers. closeConnection removes only the matching active association. getUserId remains ProtocolManager-only; Session delegates the other logical methods. No connection features are published through ResourceManager.

Hosting owns sockets, bounded buffers and its worker; ClientConnection owns client transport/state/promises. The shared socket helpers implement RAII and nonblocking platform I/O. Select waits at most 50 ms. Pending peers expire after five seconds; admitted idle peers have no heartbeat deadline. At most 32 host peers are accepted.

## Framing and Envelope

The private codec in src/protocol.hpp/.cpp uses a four-byte unsigned big-endian JSON byte length, capped at 65536, then strict UTF-8 JSON with nesting capped at 64. Complete frames are consumed in order; incomplete frames leave the buffer unchanged. Invalid frames reject without consumption. Duplicate keys, BOM, trailing data, unknown envelope fields, invalid types/strings/version and unsupported JSON values reject.

```json
{"version":3,"type":"example.command","requestId":"client-2","payload":{"value":7}}
```

The closed envelope fields are version/type/payload and optional requestId at codec level. The host requires a valid nonblank requestId on commands. Notifications from the admitted host omit requestId and use the client's command registry. Payload is always an object. Accepted replies contain exactly payload.result, which may be any JSON value; rejected replies contain code/message. Version 3 rejects retired role-based JSON v2. Both current binaries must support notifications; earlier reply-only v3 clients cannot receive the new event messages.

## Permissions and Dispatch

add_command takes CommandRequirements and a JSON callback. Permission::Any = 0 is public command access, reserved for requirements and not assignable to a participant. A PermissionSet{} requirement instead needs a registered authenticated user, with no flags. Other masks require all listed bits. Unknown access enum values or a public requirement with flags reject registration.

All names, including built-in join/resume, follow the same fresh-context lookup/dispatch path. Anonymous live peers execute explicit public commands; identified users are checked via permissions.hasPermissions at execution time. Client-supplied identity/masks do not define the actor. Target restrictions are enforced by the relevant manager. Internal permissions.* features are not automatically network RPCs.

## Admission and Persistence

session.join takes only sessionCode and returns UserId/permissions=7/original userToken. session.resume takes code/token and returns UserId/current permissions, without a new token. Code/payload/pending checks run inside their registered Any callbacks. Successful connections stay open; resume replaces the previous live connection. See session-admission.md for transactions and errors.

Permissions are numeric unsigned masks with known bits only. PermissionSet::fromMask/toMask handles validated conversion; floats, strings, booleans, negative values and unknown bits reject at JSON boundaries. Mask zero is valid for a registered user and does not imply public access. Presets are not roles or persistent labels; the immutable host has an explicit separate identity.

Version-2 archives persist hostId, users/masks and token hashes. Legacy role files reject without conversion. Credentials remain eight random bytes/16 lowercase hex characters, hashed only on the host. Permission-mutation commands save automatically through the identity coordinator, then notify only the affected user after effective changes. Direct setters still need explicit persistence and do not emit events. Client snapshots refresh on validated permissions.changed; permissions.get reads current server state. No SessionId or persisted SessionCode is added. See session-events.md for queues, routing and reconnection.

## Client and Verification

Client join/resume and generic sendCommand return futures. IDs increase across admissions and commands; the worker handles partial/coalesced I/O and exact correlation, including out-of-order command replies. At most 64 commands are outstanding with five-second deadlines. Disconnect/timeout completes remaining promises without replay. Complete results survive invalid following data. Session owns no socket logic.

Sixteen native play suites pass through the root Debug/Ninja CMake configuration on Windows, including strict framing/masks, targeted live notifications, mixed replies, queue bounds, permission RPC rollback/restart and prior stop regressions. Historical separate-process checks remain documented. POSIX remains unverified; see development.md.
