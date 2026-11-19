# Session Protocol

Status: connected-participant query/events added 2026-11-19 to JSON v3 admission, commands and targeted permission notifications. The presence snapshot includes only connected users plus the local host, with no extra permission requirement after admission. Sheet commands, participant display names and full game persistence remain separate.

## Session Code and Connections

SessionCode has exactly five ASCII A-Z/0-9 symbols and accepts lowercase by normalization. A host ProtocolManager uses the code supplied by Session when available and otherwise generates a code for that object. The desktop app stores a code in each hosted campaign manifest and supplies it on reconstruction. Clients need a reachable IPv4, port and the campaign code. Resume additionally needs the original token. A host-wide uniqueness registry remains deferred.

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

Version-2 archives persist hostId, users/masks and token hashes. Legacy role files reject without conversion. Credentials remain eight random bytes/16 lowercase hex characters, hashed only on the host. Permission-mutation commands save automatically through the identity coordinator, then notify only the affected user after effective changes. Direct setters still need explicit persistence and do not emit events. Client snapshots refresh on validated permissions.changed; permissions.get reads current server state. Identity archives still have no SessionId; campaign SessionData stores the invitation code separately. See session-events.md for queues, routing and reconnection.

session.getParticipants returns revision/participants through the ordinary command result. session.participantsChanged uses the same payload without requestId and updates the client cache. Admission schemas are unchanged; Hosting sends the normal admission response before the initial presence snapshot. Revisions are runtime-only and reset on host reconstruction; a new client connection clears its previous cache. Both binaries must support the built-in presence event. See session-participants.md.

## Client and Verification

Client join/resume and generic sendCommand return futures. IDs increase across admissions and commands; the worker handles partial/coalesced I/O and exact correlation, including out-of-order command replies. At most 64 commands are outstanding with five-second deadlines. Disconnect/timeout completes remaining promises without replay. Complete results survive invalid following data. Session owns no socket logic.

Seventeen native play suites pass through the root Debug/Ninja CMake configuration on Windows, including strict framing/masks, targeted live notifications, mixed replies, queue bounds, permission RPC rollback/restart and prior stop regressions. Historical separate-process checks remain documented. POSIX remains unverified; see development.md.
