# Session Protocol

JSON v4 carries display names in admission requests and participant snapshots. The list includes connected users plus the local host and is available to all admitted participants.

## Session Code and Connections

SessionCode has exactly five ASCII A-Z/0-9 symbols and accepts lowercase by normalization. A host ProtocolManager uses the code supplied by Session when available and otherwise generates a code for that object. The desktop app stores a code in each hosted campaign manifest and supplies it on reconstruction. Clients need a reachable IPv4, port and the campaign code. Resume additionally needs the original token. A host-wide uniqueness registry remains deferred.

ProtocolManager owns logical ConnectionIds, pending records and active UserId bindings. registerConnection allocates nonzero monotonically increasing IDs. bindUser associates an identity verified by its caller and replaces any previous binding. makeContext checks existence/both directions; closed/replaced peers are not anonymous peers. closeConnection removes only the matching active association. getUserId remains ProtocolManager-only; Session delegates the other logical methods. No connection features are published through ResourceManager.

Hosting owns sockets, bounded buffers and its worker; ClientConnection owns client transport/state/promises. The shared socket helpers implement RAII and nonblocking platform I/O. Select waits at most 50 ms. Pending peers expire after five seconds; admitted idle peers have no heartbeat deadline. At most 32 host peers are accepted.

## Framing and Envelope

The private codec in src/protocol.hpp/.cpp uses a four-byte unsigned big-endian JSON byte length, capped at 65536, then strict UTF-8 JSON with nesting capped at 64. Complete frames are consumed in order; incomplete frames leave the buffer unchanged. Invalid frames reject without consumption. Duplicate keys, BOM, trailing data, unknown envelope fields, invalid types/strings/version and unsupported JSON values reject.

```json
{"version":4,"type":"example.command","requestId":"client-2","payload":{"value":7}}
```

The closed envelope fields are version/type/payload and optional requestId at codec level. The host requires a valid nonblank requestId on commands. Notifications from the admitted host omit requestId and use the client's command registry. Payload is always an object. Accepted replies contain payload.result and rejected replies contain code/message. Both peers must use JSON v4.

## Permissions and Dispatch

add_command takes CommandRequirements and a JSON callback. Permission::Any = 0 is public command access, reserved for requirements and not assignable to a participant. A PermissionSet{} requirement instead needs a registered authenticated user, with no flags. Other masks require all listed bits. Unknown access enum values or a public requirement with flags reject registration.

All names, including built-in join/resume, follow the same fresh-context lookup/dispatch path. Anonymous live peers execute explicit public commands; identified users are checked via permissions.hasPermissions at execution time. Client-supplied identity/masks do not define the actor. Target restrictions are enforced by the relevant manager. Internal permissions.* features are not automatically network RPCs.

## Admission and Persistence

session.join takes sessionCode and displayName and returns UserId, mask 7 and the original userToken. session.resume takes code, token and an optional displayName and returns the existing UserId and current permissions. When a name changes, storage commits before binding replacement and presence publication. See session-admission.md.

Permissions are numeric unsigned masks with known bits only. PermissionSet::fromMask/toMask handles validated conversion; floats, strings, booleans, negative values and unknown bits reject at JSON boundaries. Mask zero is valid for a registered user and does not imply public access. Presets are not roles or persistent labels; the immutable host has an explicit separate identity.

Version-4 archives persist hostId, user IDs, masks, display names and tokens. Earlier versions are rejected. Credentials remain eight random bytes/16 lowercase hex characters, stored directly on the host. Permission mutations save automatically and notify the affected user. See session-tokens.md and session-events.md.

session.getParticipants returns revision/participants through the ordinary command result. session.participantsChanged uses the same payload without requestId and updates the client cache. Each participant entry contains userId, displayName and isHost. Hosting sends the admission response before the initial presence snapshot. Revisions are runtime-only and reset on host reconstruction. See session-participants.md.

## Client and Verification

Client join/resume and generic sendCommand return futures. IDs increase across admissions and commands; the worker handles partial/coalesced I/O and exact correlation, including out-of-order command replies. At most 64 commands are outstanding with five-second deadlines. Disconnect/timeout completes remaining promises without replay. Complete results survive invalid following data. Session owns no socket logic.

Seventeen native play suites pass through the root Debug/Ninja CMake configuration on Windows, including strict framing/masks, targeted live notifications, mixed replies, queue bounds, permission RPC rollback/restart and prior stop regressions. Historical separate-process checks remain documented. POSIX remains unverified; see development.md.
