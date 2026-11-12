# Removed Game/Network Prototype

Archived 2026-11-10. These excerpts describe the removed prototype and its earlier verification, not the current backend or launch instructions. Game, GameHost/GameClient, their console client, and prototype-specific tests have been deleted. Current code uses Session, ProtocolManager, the JSON codec, and private Hosting; see ../session-protocol.md and ../development.md.

At removal on 2026-11-10, the replacement host handled only pending peers. Later JSON admission is documented in ../session-admission.md; this archive does not describe that implementation. Client-side ProtocolManager remains separate. The Linux send-timeout ordering below is a prior observation to verify again when implementing that client; earlier Tailscale results do not validate the new architecture.

## Historical TCP Presence Slice

- `core/play/` is an independent C++20 backend module, built as `golarion_play`. It depends on nlohmann_json 3.12 for commands and JSON-only peers, not on `core/sheet/`, a GUI toolkit, or a third-party networking library. It can be built standalone with the JSON package.
- `Game` validates its ID/name and participant display names, assigns per-run participant IDs, maintains the currently connected roster, and returns detached `GameSnapshot` values. Different participants may have the same display name. Disconnect removes live presence; it does not implement persistent membership. A new connection currently receives a new identity.
- `GameHost` and `GameClient` use IPv4 TCP sockets with RAII ownership and polling APIs. The host handles multiple clients in one event loop, broadcasts complete presence snapshots after joins/departures, bounds message buffers, rejects malformed joins, and removes incomplete handshakes after five seconds. The initial transport caps concurrent sockets at 32 (or a lower platform descriptor limit), and send attempts have a one-second socket timeout; this is a basic synchronous polling transport, not a general production networking service.
- Configure the client's send timeout only after TCP connection succeeds. On Linux, `SO_SNDTIMEO` also limits `connect()` and may return `EINPROGRESS` (115 on the tested system) when a delayed connection exceeds one second; it is not evidence that the host accepted the connection. A loopback socket reproduction on Ubuntu/WSL confirmed the premature error with the old ordering and successful delayed connection with the corrected ordering. The native Windows multi-process test also passes after this fix; the C++ POSIX build remains unverified because the local Ubuntu environment has no C++ compiler.
- The private JSON v2 codec carries a four-byte big-endian byte length and UTF-8 envelope. The diagnostic contract uses `game.join`, correlated `request.accepted`/`request.rejected`, and `game.snapshot`; schemas are validated in the transport adapter. Limits remain 64 KiB per frame and 128 bytes for prototype text/request IDs. Legacy binary is rejected; see `session-protocol.md`.
- `golarion_play_host` and `golarion_play_client` are diagnostic console executables for testing the backend, not the future frontend. Both require the address first and the port second: `golarion_play_host <IPv4 address> <port> [game name]` and `golarion_play_client <host address> <port> <participant name>`. A host process is not itself a participant, so run a client on that PC as well to join. The diagnostic game ID is currently `game`.
- This slice has no authentication, encryption, persisted game data, stable returning identities, roles, or sheets. It is a connectivity/presence prototype; protected Internet hosting remains part of the later design. No firewall/router configuration is modified by the tools.
- `play_game_test` and `play_json_protocol_test` cover the model and JSON framing. `play_network_process_test` starts real peers and simulated hosts on loopback, checking presence, UTF-8/fragmentation, correlated replies, invalid request and snapshot schemas, repeated join, legacy rejection, departures/rejoin, and shutdown. Python is only the test runner. The obsolete binary test is removed.
- Windows verification used GCC 10.3 with C++20 and warnings enabled: both C++ tests and the real process test passed. The sandbox blocked loopback connects with WSA error 10013; the process test passed when executed with local socket permissions. The user also confirmed a real Windows-host/Linux-client connection over Tailscale after the reachability check and scoped firewall-rule instructions. CMake is not installed in the current Windows environment, so the CMake configuration itself was not run; the full automated C++ POSIX suite remains unverified locally.

See `development.md` for standalone build and launch commands. The following sections describe later game-management behavior, not additional implemented features.

## Removed Diagnostic Message Contract

These schemas belong to the existing Game/GameSnapshot diagnostic tools; they do not implement persistent Session admission or RoleManager commands.

| Direction | Type | Request ID | Exact payload |
| --- | --- | --- | --- |
| Client to host | `game.join` | Required | `displayName` string. |
| Host to joining client | `request.accepted` | Same as join | `participantId` string. |
| Host to rejected connection | `request.rejected` | Echoed when valid/recoverable; otherwise omitted | `code` and `message` strings. Currently `INVALID_REQUEST` and an Italian explanation. |
| Host to joined clients | `game.snapshot` | Omitted | `id`, `name`, `connectedParticipants`; each participant has exactly `id` and `name`. |

Prototype request IDs and payload text fields are limited to 1-128 UTF-8 bytes without ASCII control characters; blank text is rejected. Snapshot arrays are capped at 1024 and duplicate participant IDs are rejected; duplicate display names remain allowed. Unknown or extra payload fields and wrong field types are rejected. The common codec still applies its own global limits.

The host confirms admission before broadcasting a snapshot. The current C++ client makes only one join request per connection, using `join-1`, verifies the reply correlation, and requires acceptance before any snapshot. It checks that its own participant ID appears in each snapshot and drains coalesced confirmation/event frames. A rejection terminates that client connection. Host validation errors close only the offending connection and preserve other participants. An invalid/oversized request ID is never echoed into an error response.

`play_network_process_test` verifies native peers, UTF-8 names, fragmented joins, confirmation ordering/correlation, malformed payloads, absent/oversized IDs, unknown messages, old versions/binary packets, repeated join rejection, abrupt departure/rejoin, and host shutdown. Simulated hosts test the native client against coalesced replies/events, wrong IDs/order, duplicate confirmations, corrupt/oversized participant lists, absent own identity, and valid subsequent updates. The public C++ network API and IP/port console arguments remain unchanged.
