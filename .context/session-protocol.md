# Session Communication Protocol

Status: updated 2026-11-12. ProtocolManager owns connections, TCP hosting, persisted identities, JSON admission, and authenticated dispatch of registered commands. Session remains a facade. No display names are stored. Host-wide code uniqueness/persistence are deferred. Game/network and their old client/tests are removed. Client-side ProtocolManager, concrete role/sheet commands, and user-list events remain future work. See `session-admission.md` and `session-commands.md`.

## Implemented Session Code

- `SessionCode` is a small value type in `include/golarion/play/session_code.hpp` and `src/session_code.cpp`. Its constructor accepts exactly five ASCII letters/digits and stores uppercase `A-Z0-9`; malformed input throws `std::invalid_argument`. Spaces are not trimmed, and controls, punctuation, non-ASCII bytes, and wrong lengths are rejected.
- `SessionCode::generate()` creates a candidate by sampling the 36-character alphabet with `std::random_device` and `std::uniform_int_distribution`. It does not check collisions or reserve a code. Random-source failures propagate instead of installing a fallback code. The user explicitly limited this step to generation/comparison; the shared host registry and persistence are separate future changes.
- `value()` returns a view into the value object; `matches(candidate) noexcept` uses the same ASCII normalization and returns false for malformed or different input without changing the stored code.
- ProtocolManager owns a const SessionCode generated once during construction, before feature registration. `getSessionCode()` returns a detached string and `verifySessionCode(candidate) noexcept` delegates to matches. Session exposes only a delegating getSessionCode for display. No code feature is registered in ResourceManager.
- The value stays unchanged across failed starts, active hosting, stop, and restart within that Session's lifetime. Identity restoration does not restore SessionCode, and a host-wide uniqueness guarantee is not implemented. SessionCode can parse an existing value, but wiring saved codes into Session construction belongs to separate code-persistence work.
- The host console prints `Codice sessione: XXXXX` after the listening-address line. Both JSON join and resume verify this current code; it is not the personal user credential.
- `play_session_code_test` covers fixed/generated candidates, uppercase normalization, case-insensitive matching, malformed/non-ASCII input, and value ownership. ProtocolManager tests cover its direct getter/verifier, while native hosting tests cover code stability through stop/restart and startup failure. No test assumes random candidates are unique.

## Implemented Logical Connection API

The following methods and their connection maps/counter belong to ProtocolManager. Session exposes delegations for registration, binding, context construction, and close; `getUserId` exists only on ProtocolManager. Connection operations are not published through ResourceManager, since no other subsystem needs them.

- `registerConnection()` returns a fresh nonzero ConnectionId for a pending logical connection. IDs are never reused within the same ProtocolManager lifetime; exhaustion throws `std::overflow_error` rather than recycling them.
- `bindUser(connectionId, userId)` returns the optional previous ConnectionId to close physically and immediately removes its logical record. Repeating the same binding is idempotent; changing an already-bound connection's user is rejected. Unknown/closed/replaced IDs and empty or ASCII whitespace/control-containing UserIds throw `std::invalid_argument` before modifying existing bindings. UserIds are exact and case sensitive.
- `getUserId(connectionId) const` returns a detached optional UserId only when both binding directions match. Pending/unknown/closed/replaced connections return no user. This direct Hosting lookup is not sufficient to distinguish a pending record from a removed record; no connection lookup feature is published through ResourceManager.
- `makeContext(connectionId) const` checks the actual connection record: live pending returns `CommandContext{std::nullopt, connectionId}`, active admitted returns its UserId/ConnectionId, and unknown/closed/replaced records throw `ProtocolError(NotAdmitted)`. It does not authenticate or cache roles. Returned values remain detached; dispatch revalidates record existence and optional-identity equality, rejecting stale contexts including cached anonymous contexts after admission.
- `closeConnection(connectionId)` removes the logical record and only its matching reverse association. Unknown/closed/replaced IDs return false, so a late disconnect cannot remove a replacement. It does not change roles or directly manipulate sockets; the hosting worker notices removed records and closes their sockets.

`bindUser` is a trusted internal association operation. Its caller must first verify identity and arrange role registration. It checks ID format and binding invariants without creating an identity or verifying tokens. JSON resume verifies the token before calling it; join stages a binding together with `roles.registerUserWithCommit` and persistence. ProtocolManager independently rejects unknown users when dispatching.

Session owns only ResourceManager, ProtocolManager, and RoleManager, with the registry outliving providers. Logical connection methods use a mutex shared with the hosting worker. No registry/subsystem getters are added. `play_protocol_manager_test` directly covers pending/unique IDs, independent users/managers, idempotence, replacement, stale requests/disconnects, invalid identity switches, detached data, close/rebind, and fresh-context dispatch with live role checks. `play_session_test` checks delegated lifecycle behavior and independent Sessions. These suites and RoleManager pass with GCC C++20 and warnings treated as errors; CMake remains unavailable locally.

## Implemented Hosting Boundary

Session exposes `std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port)` and `void stopHosting()` as direct delegations to ProtocolManager. A numeric IPv4 listener and dedicated background worker start before the actual port is returned; port 0 is supported. There is no application-side polling. Explicit stop joins the worker, releases sockets, and clears logical connection/binding maps without resetting IDs or changing roles. Both Session and ProtocolManager stop automatically on destruction; Session does so before provider teardown. See `protocol-manager.md` for lifecycle error handling and synchronization.

The listener accepts pending connections, assigns ConnectionIds, and forwards decoded messages to ProtocolManager's common command handler. Successful join/resume keeps the socket open. The five-second deadline applies only before admission; successful generic Any commands do not reset it or admit the peer. Multiple buffered requests are processed in order, with one bounded response outstanding per peer. Input buffering is capped at 65536 + 4 bytes and total TCP peers at 32 (and platform select capacity). Sockets remain nonblocking and select waits at most 50 ms. Exact admission schemas, rejections, persistence coordination, and replacement behavior are in `session-admission.md`.

Every request executes through the same registry/dispatch, including join/resume registered with minimum Any. Pending peers can execute Any; requests requiring Spectator or higher receive `FORBIDDEN`, and unknown names receive `UNKNOWN_COMMAND`. Pending errors close the peer; ordinary admitted errors keep it open. Every success uses `request.accepted` with `payload.result`. See `session-commands.md`. No user snapshots or shutdown notices are sent. The console configures an archive but has no concrete game-command provider or client executable yet.

`play_hosting_test` verifies native Windows loopback peers, startup failures, fragmentation/invalid input, pending-only replies, shutdown/restart, concurrent state/lifecycle access, role retention, and destruction cleanup. The sandbox denies loopback connects with WSA error 10013; the test passes with local socket permissions. The POSIX branch remains unverified locally.

## Implemented ProtocolManager Boundary

- Only `protocol.add_command` is exposed through ResourceManager. `dispatch` is a direct C++ method, and there is no command-removal API. The implemented callback receives `CommandContext` and an object JSON payload and returns a JSON result, as confirmed by the user.
- Role checks always use RoleManager at execution time, including Any. `Host > Master > Player > Spectator > Any`; Any is only an access threshold for anonymous peers and cannot be assigned or persisted. Supplied unknown identities are not anonymous. Subsystem mutations still enforce target-specific rules. Commands remain registered throughout the Session's lifetime, and dispatch must stop before providers are destroyed.
- ProtocolManager constructs fresh context from active bindings and reuses dispatch for network commands. Framing, admission, persistence, and authenticated results/rejections are implemented. Concrete role/sheet commands remain future work; see `protocol-manager.md` and `session-commands.md`.

Wire context contains optional UserId and ConnectionId, is created by trusted host code at execution time, and is revalidated by dispatch. Execution is serialized with replacement/close, preventing stale contexts from starting afterward. Local trusted calls require a known UserId and use the same permission checks without a socket; a context with neither identity nor connection is rejected. Nested registration/dispatch remains supported; see `session-commands.md` for synchronization and lifecycle constraints.

## Confirmed Connection-to-User Binding

Logical registration/binding/replacement/context/close, TCP hosting, identity/token persistence, and remote join/resume are implemented:

- ProtocolManager owns logical connection bindings directly, without a separate ConnectionManager. Session remains a container/facade. Physical sockets are RAII-owned by its private Hosting implementation; roles remain authoritative in RoleManager. Only operations needed by other subsystems are published through ResourceManager.
- ProtocolManager's `registerConnection` assigns logical ConnectionIds and is called by the worker on each accept. IDs are distinct from UserId, IP, and OS descriptors, so recycled descriptors cannot represent old connections.
- ProtocolManager's logical connection record has an optional UserId, empty until admission succeeds. A reverse UserId-to-current-ConnectionId index enforces one active connection per identity and refers to the corresponding live transport socket.
- ProtocolManager owns the private per-Session token-to-UserId registry and archive operations; see `session-tokens.md`. Join uses staged Player registration, credential issuance, and archive writing as one operation; resume uses `resolveUserToken` without re-registering or resetting the role. RoleManager does not own sockets or credentials, and Session only delegates.
- Admission validates identity before binding. Remote command handling uses a fresh `makeContext`; actor identity/role never comes from the payload. Active lookup remains a direct ProtocolManager method rather than a ResourceManager feature.
- Resume invalidates the old connection before it can dispatch further commands and installs the replacement binding. Late requests/disconnect callbacks from the old ConnectionId must fail the current-binding check and must not disconnect the replacement.
- Ordinary disconnect removes only the matching active connection binding. It preserves the user's identity, token, and assigned role. Until admitted, a live connection may execute only commands registered with minimum Any; only join/resume establish an identity.

## Confirmed Requirements

- Connection requires the host IP/port and the session's random five-character code from `A-Z` and `0-9`. Lowercase ASCII letters are accepted and converted to uppercase. The required uniqueness scope is the sessions managed by the same host, not independent hosts. Generation/comparison are implemented; enforcing that scope awaits the shared host registry.
- JSON messages travel over TCP.
- The host issues a personal token at the first connection of a new client. The token identifies that user within this particular `Session`, including after closing and reopening it.
- A returning client reuses its token; reconnecting must not generate a new identity or reset the assigned role.
- One user identity has at most one active connection. A successful resume with a valid token replaces its previous live connection, preserving the user ID, token, and current role.
- The hosting user has the immutable `Host` role. New users start as `Player`; role changes follow the confirmed policy in [game-management.md](game-management.md#confirmed-structural-decisions-for-later-steps).
- Role data is synchronized internally; see `session-roles.md`. ProtocolManager handles admission and authenticated registered commands on its worker. Finish registry registration before hosting and stop before teardown. Command registration/execution and binding changes are serialized; independent provider access retains its own thread-safety contract. No Session polling is needed.

## Implemented JSON Codec and Envelope

`core/play/src/protocol.hpp` declares `detail::ProtocolMessage` with `type`, optional `requestId`, and object JSON `payload`. `encodeMessage` serializes it as UTF-8 with a four-byte unsigned big-endian payload length. The limit remains 65536 JSON bytes, excluding the prefix. `decodeMessage(buffer)` extracts exactly one complete valid message; incomplete input returns `nullopt`, and both incomplete and invalid input leave the buffer unchanged. Remaining frames stay buffered for subsequent calls.

The codec writes and requires integer version `2`; booleans, floats, numeric strings, unsupported versions, and values that would wrap during narrowing are rejected. The JSON root must have exactly `version`, `type`, `payload`, and optionally `requestId`. Type/requestId must be nonblank strings; payload must be an object. Names/IDs are preserved without normalization, and command names are not restricted to a fixed list. Requiring requestId for a particular request/response and validating each payload's schema remain application responsibilities.

Input rejects malformed JSON/UTF-8, BOM, trailing non-whitespace data, unknown envelope fields, duplicate keys within any object, and more than 64 nested containers including the envelope. Outgoing payloads are checked iteratively before copying/serialization: binary/discarded values and non-finite numbers are rejected instead of silently changing their representation. Invalid UTF-8 strings/keys are rejected during serialization. Validation failures throw `std::invalid_argument`.

Only the JSON codec remains. Its header has no domain-model or variant dependency. Peers reject legacy binary packets and JSON versions other than 2; there is no compatibility/negotiation layer. ProtocolManager's admission handler owns payload schemas, while Hosting owns framing and socket buffers.

`play_json_protocol_test` passes with GCC C++20 and warnings treated as errors: round trips, UTF-8, independent wire input, fragmentation/concatenation, envelope validation, duplicate keys, unsupported values, nesting/size limits, unchanged buffers, and complete legacy packet rejection are covered. Native hosting tests verify pending-peer TCP handling and the admission suite verifies join/resume and persistent streams. Removed prototype tests are recorded in `history/2026-11-10-game-network-prototype.md`. CMake remains unavailable locally.

Common fields:

| Field | Meaning |
| --- | --- |
| `version` | Integer protocol version, initially `2`. |
| `type` | Stable English message type, such as `session.join` or `users.setRole`. |
| `requestId` | Client-generated string identifying a request, echoed in its response. Events omit it. It correlates responses; it does not by itself guarantee durable deduplication. |
| `payload` | Object with the data for this message type. |

Do not send C++ object memory, opaque handles, or persistence structs as implicit wire formats. The codec checks the envelope, dispatch checks names/current permissions, and providers validate payloads/domain rules. Fields/roles/errors are English; user-facing messages are Italian. Results are wrapped in `payload.result`, including scalars/null; unencodable results produce `COMMAND_FAILED` without retry.

## Implemented Tokens and Identity Persistence

ProtocolManager issues eight random bytes encoded as 16 lowercase hexadecimal characters using OpenSSL Crypto. It persists stable UserIds, roles, and SHA-256 token hashes in a versioned JSON archive, without a SessionId or original credentials. The client needs only its original token. Session delegates registration, issuance, save, and load; lookup stays on ProtocolManager. Issuance requires a saved/loaded archive and saves before returning the token. Load requires stopped hosting and the same immutable Host UserId.

See `session-tokens.md` for archives, `session-admission.md` for join/resume, and `session-commands.md` for authenticated dispatch. Role changes still require an explicit save. Client storage, concrete role/sheet commands, and host-wide SessionCode uniqueness/persistence remain deferred.

## Pending Client Support

ProtocolManager currently implements the host side, including admission. Add client-side connection/disconnection, JSON request transmission and response correlation, token retention, and reception of future host events/snapshots in a separate step. The client API and its integration with Session remain to be designed. There is no replacement console client yet.

## Proposed Persistent Session Message Set

Join/resume, generic command routing, and correlated accepted/rejected outcomes are implemented. Concrete role commands, other session messages, and user-list synchronization below remain future work. Display names are deferred.

| Direction | Type | Purpose |
| --- | --- | --- |
| Client to host | `session.join` | Implemented: first admission using only `sessionCode`; creates a new `Player`. |
| Client to host | `session.resume` | Reconnect using `sessionCode` and `userToken`; restores the known identity and its current role. |
| Client to host | `users.setRole` | Request a role change for `targetUserId`, specifying `role`. |
| Client to host | `session.leave` | Close this live connection; retain the user identity/token for later resume. |
| Client/host | `system.ping`, `system.pong` | Liveness messages; intervals/timeouts remain to be agreed. |
| Host to requester | `request.accepted`, `request.rejected` | Implemented for every command: result in `payload.result` or stable rejection code/Italian message. Join result is `{userId, role, userToken}`; resume result is `{userId, role}`. |
| Host to connected clients | `users.snapshot` | Complete current connected-user list and monotonically increasing revision after a join, leave, or role change. |
| Host to connected clients | `session.closing` | Orderly server shutdown notice. Socket failure still handles unannounced shutdown. |
| Host to replaced connection | `session.replaced` | Best-effort notice before closing the old connection after a successful resume elsewhere; the new connection does not wait for an acknowledgement. |

A future connected-user snapshot must omit tokens and represent the host once even though it does not need a remote client connection. Current admission returns only the requester's identity/role, plus a new credential on join; display names are not part of the implemented model. An ordinary client cannot request the `Host` role, and Any is never a participant role. The former `session.joined` and `session.resumed` response types are retired without changing the JSON envelope version.

## Proposed Admission and Identity Flow

1. Establish TCP, then submit `session.join` or `session.resume` to acquire an identity. Other explicitly registered Any commands may run while still pending, within the original admission deadline.
2. The host uses the common registry and role check (Any), then the admission callback checks its schema, current session code, and any supplied personal token.
3. A new admitted client receives an assigned user ID, `Player` role, and a freshly generated opaque personal token.
4. The host associates the accepted connection with that identity. Subsequent requests do not choose their acting user ID or assert their own role.
5. A reconnecting client uses `session.resume`, retains its identity, and receives the role currently stored by the host. Invalid/foreign tokens are rejected instead of silently creating a new player. Only after successful validation does the host bind the identity to the new connection and revoke/close the previous connection, if any.
6. The host sends the private result to the requester. Publishing connected-user snapshots to other recipients remains a later step.

The shared session code will identify the session and admit access at the given endpoint; it is not the personal user identity. Candidate generation/comparison already use canonical uppercase `A-Z0-9`. The future assignment/storage step must check candidates against the sessions already managed by that host and regenerate on a collision. The host's persistent session registry must preserve the uniqueness constraint across restarts. Different hosts may use the same code because the host endpoint is also required.

After connection replacement, the old connection must no longer execute requests for that user. A late close/disconnect callback from the old socket must not mark the replacement connection offline. The public connected-user list contains one entry per identity, not one entry per historical socket. Replacing a connection does not rotate the permanent token.

Permanent tokens require persistence on both sides: the client must retain its credential and the host must retain the per-session token-to-user mapping and role. Host-side persistence and the eight-byte token format are implemented in `session-tokens.md`; client storage and any later revocation operation remain future work. Tokens are opaque random credentials and do not encode a client-selected identity or role. Only the individual join reply will return its newly issued token; public user snapshots and role-change events must not.

## Proposed Role Request and Outcome

```json
{
  "version": 2,
  "type": "users.setRole",
  "requestId": "request-42",
  "payload": {
    "targetUserId": "user-7",
    "role": "spectator"
  }
}
```

The host determines the actor from the accepted connection and checks current actor/target roles at execution time. Host-originated local operations follow the same domain permission checks. `Host` is immutable; assigning `Master` or changing an existing master's role requires the host. A master may only change other players/spectators between those two roles.

On success, reply `request.accepted` with the resulting user-list revision, then publish `users.snapshot`. On rejection, reply `request.rejected` with the same `requestId` and no state change. Proposed error codes include `INVALID_SESSION_CODE`, `INVALID_USER_TOKEN`, `UNSUPPORTED_VERSION`, `INVALID_REQUEST`, `UNKNOWN_USER`, `FORBIDDEN`, and `HOST_ROLE_IMMUTABLE`.

The first version should send complete user lists rather than incremental patches. A new/resumed client gets a current snapshot; its cached role/list is never authoritative. A missing response must not cause an automatic replay of an arbitrary old request after reconnecting; resume and fetch current state first. More elaborate deduplication can be specified when non-idempotent game operations are added.

## Remaining Session Admission Decisions

- Heartbeat timing and display-name rules. Current admission payloads, five-second pending deadline, and token format are implemented; any future revision is a separate step.
- Client credential persistence and protected transport. Host identity persistence and JSON admission are implemented; client integration remains pending.

References: [TCP stream/message boundaries, RFC 9293 section 3.7](https://www.rfc-editor.org/rfc/rfc9293.html#section-3.7) and [JSON UTF-8 encoding, RFC 8259 section 8.1](https://www.rfc-editor.org/rfc/rfc8259.html#section-8.1).
