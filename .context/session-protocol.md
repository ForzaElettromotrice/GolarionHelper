# Session Communication Protocol

Status: protocol design, 2026-11-07. JSON over TCP, session-code rules, permanent per-session user tokens, and connection replacement are confirmed. The detailed message contract below is proposed for review; it is not implemented. `Session` owns its function registry and the independently tested RoleManager model; the diagnostic TCP prototype still uses its original binary protocol.

## Confirmed Requirements

- Connection requires the host IP/port and the session's random five-character code from `A-Z` and `0-9`. Lowercase ASCII letters are accepted and converted to uppercase. Codes are unique among the sessions managed by the same host; uniqueness across independent hosts is not required.
- JSON messages travel over TCP.
- The host issues a personal token at the first connection of a new client. The token identifies that user within this particular `Session`, including after closing and reopening it.
- A returning client reuses its token; reconnecting must not generate a new identity or reset the assigned role.
- One user identity has at most one active connection. A successful resume with a valid token replaces its previous live connection, preserving the user ID, token, and current role.
- The hosting user has the immutable `Host` role. New users start as `Player`; role changes follow the confirmed policy in [game-management.md](game-management.md#confirmed-structural-decisions-for-later-steps).
- Role-management domain functions are implemented independently of transport; see `session-roles.md`. Hosting and protocol integration remain later steps. Starting hosting is a local `Session` operation, not an incoming network command.

## Proposed Framing and Envelope

Retain the prototype's four-byte unsigned big-endian length prefix, followed by exactly that many payload bytes. For the next protocol, the payload is one UTF-8 JSON object without a byte-order mark. Initially retain the 64 KiB payload limit. Reads may contain part of a frame or several frames; the decoder must buffer and extract complete messages before dispatch.

Use protocol version `2`, because the JSON body replaces the prototype's version-1 binary payload. An incompatible message must not join or mutate the session. The prototype and the new protocol require matching host/client versions; no automatic compatibility layer is proposed.

Common fields:

| Field | Meaning |
| --- | --- |
| `version` | Integer protocol version, initially `2`. |
| `type` | Stable English message type, such as `session.join` or `users.setRole`. |
| `requestId` | Client-generated string identifying a request, echoed in its response. Events omit it. It correlates responses; it does not by itself guarantee durable deduplication. |
| `payload` | Object with the data for this message type. |

Do not send C++ object memory, opaque C handles, or persistence structs as implicit wire formats. Each message needs an explicit schema, including required fields and limits. Unknown types, invalid field types, oversized messages, and unsupported versions are rejected before domain mutation. Machine-readable field names, role values, and error codes are English; client-visible labels/messages remain Italian.

## Proposed First Message Set

| Direction | Type | Purpose |
| --- | --- | --- |
| Client to host | `session.join` | First admission using `sessionCode` and `displayName`; creates a new `Player`. |
| Client to host | `session.resume` | Reconnect using `sessionCode` and `userToken`; restores the known identity and its current role. |
| Client to host | `users.setRole` | Request a role change for `targetUserId`, specifying `role`. |
| Client to host | `session.leave` | Close this live connection; retain the user identity/token for later resume. |
| Client/host | `system.ping`, `system.pong` | Liveness messages; intervals/timeouts remain to be agreed. |
| Host to requester | `session.joined` | Own `userId`, newly issued `userToken`, `role`, current user-list revision, and connected-user snapshot. |
| Host to requester | `session.resumed` | Restored `userId`/`role` and current connected-user snapshot/revision. |
| Host to requester | `request.accepted`, `request.rejected` | Outcome of a request, echoing `requestId`; rejection includes a stable error code. |
| Host to connected clients | `users.snapshot` | Complete current connected-user list and monotonically increasing revision after a join, leave, or role change. |
| Host to connected clients | `session.closing` | Orderly server shutdown notice. Socket failure still handles unannounced shutdown. |
| Host to replaced connection | `session.replaced` | Best-effort notice before closing the old connection after a successful resume elsewhere; the new connection does not wait for an acknowledgement. |

A connected-user entry contains only `userId`, `displayName`, and `role`. It never includes a token. The host is represented once in the user model even though it does not need a remote client connection. An ordinary client cannot request the `Host` role.

## Proposed Admission and Identity Flow

1. Establish TCP and submit `session.join` or `session.resume` as the first application request.
2. The host checks protocol/schema, the current session code, and any supplied personal token.
3. A new admitted client receives an assigned user ID, `Player` role, and a freshly generated opaque personal token.
4. The host associates the accepted connection with that identity. Subsequent requests do not choose their acting user ID or assert their own role.
5. A reconnecting client uses `session.resume`, retains its identity, and receives the role currently stored by the host. Invalid/foreign tokens are rejected instead of silently creating a new player. Only after successful validation does the host bind the identity to the new connection and revoke/close the previous connection, if any.
6. The host sends the private result to the requester and publishes the current connected-user snapshot to other recipients.

The shared session code identifies the session and admits access at the given endpoint; it is not the personal user identity. Generate and store it using the canonical uppercase `A-Z0-9` alphabet. Check new candidates against the sessions already managed by that host and regenerate on a collision. The host's persistent session registry must preserve the uniqueness constraint across restarts. Different hosts may use the same code because the host endpoint is also required.

After connection replacement, the old connection must no longer execute requests for that user. A late close/disconnect callback from the old socket must not mark the replacement connection offline. The public connected-user list contains one entry per identity, not one entry per historical socket. Replacing a connection does not rotate the permanent token.

Permanent tokens require persistence on both sides: the client must retain its credential and the host must retain the per-session token-to-user mapping and role. Memory-only socket state cannot satisfy the confirmed restart behavior. Tokens are opaque random credentials and must not encode a client-selected identity or role. Token length/generation/storage and any later revocation operation need an implementation design. Only the individual join reply returns its newly issued token; public user snapshots and role-change events do not.

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

## Remaining Decisions Before Implementation

- Heartbeat timing and exact name/token/schema limits, including time allowed for initial admission.
- Credential persistence and protected transport. The current tests use a private VPN; the existing prototype itself still has no authentication or encryption. The permanent-token protocol is not complete public-Internet protection by itself.

References: [TCP stream/message boundaries, RFC 9293 section 3.7](https://www.rfc-editor.org/rfc/rfc9293.html#section-3.7) and [JSON UTF-8 encoding, RFC 8259 section 8.1](https://www.rfc-editor.org/rfc/rfc8259.html#section-8.1).
