# Game Management and Application Communication

Status: TCP presence prototype, Session-owned function registry, and RoleManager implemented, 2026-11-07. The role model is tested independently; it is not integrated into the prototype's network protocol. The larger persistent-game design remains incremental. Confirmed requirements and structural decisions come from the user; remaining proposed behavior is subject to review.

## Confirmed Requirements

- A native desktop application running on PCs, developed in C++.
- A persistent game supports one or more game masters, one or more players, and optional spectators; it can be reopened across multiple play sessions.
- Participants connect from separate PCs over LAN or the Internet.
- A participant's PC hosts the game. The game is reachable while the hosting application is open; its saved data survives closing the application.
- Direct Internet access requiring network configuration, or a VPN, is acceptable for the initial version. A managed online relay or always-on dedicated server is not required.
- Players may import character sheets created locally in the application and modify sheets assigned to them. Masters may modify all game sheets.
- Communication currently means inter-application networking for joining and interacting with a shared game. In-app chat, voice, and video are outside the current scope.
- Implementation proceeds in small backend-only steps. A real TCP host with multiple connected C++ clients and synchronized presence is implemented, followed by Session, its function registry, and RoleManager; the frontend remains deferred.
- The implemented rules engine remains Pathfinder 1e. The user's general reference to DnD does not select another ruleset.

## Confirmed Structural Decisions for Later Steps

- `Session` is the main play aggregate, analogous to `CharacterSheet`, and will contain play subsystems. A user creates a session and will later be able to start its server. The first subsystem will manage connected users.
- `Session`, in `core/play/include/golarion/play/session.hpp`, now requires a host user ID and owns private ResourceManager and RoleManager members, passing the registry by reference. Hosting, the new protocol, and the connected-user/identity subsystem remain future steps.
- RoleManager publishes `roles.registerUser`, `roles.getRole`, `roles.hasAtLeastRole`, and `roles.setRole` through the Session-owned ResourceManager. It creates the immutable host and registers other stable user IDs as Player; queries and role mutations enforce the confirmed policy. See `session-roles.md` for signatures, lifecycle, validation, and tests. ProtocolManager will later publish `add_command(name, minimumRole, callback)` and enforce the trusted sender's current minimum role before dispatch. See `architecture.md` for the registry API and separation from the sheet ResourceManager.
- A session must generate a random five-character code using `A-Z` and `0-9`, supplied alongside the host IP and port to join. Lowercase input is normalized to uppercase. Codes are unique among the sessions managed by the same host, not globally across independent hosts.
- The hosting user has the unique, immutable `Host` role with master privileges. A new incoming user starts as `Player`. Only the host may promote a user to `Master` or change an existing master's role. Masters may change other players/spectators between `Player` and `Spectator`; they cannot modify another master or the host. Players and spectators cannot change roles.
- Communication will use JSON over TCP. The host issues a permanent personal token at the first admission of a new client; it identifies that user within the same `Session`, including after session restarts, and preserves their current assigned role on resume. The proposed message contract and remaining protocol details are in [session-protocol.md](session-protocol.md).
- A valid resume replaces any previous live connection for that identity, preserving the permanent token and current role. There is one active connection and one connected-user entry per identity; stale messages/disconnects from the replaced connection cannot affect the replacement.
- Store sheet IDs and names in the game registry; connect actual `CharacterSheet` content through the later application service.
- A sheet may have several authorized players; masters retain access to all game sheets.
- All masters can administer the permitted player/spectator role changes; master promotion/demotion is reserved to the immutable host. Previously discussed shared-sheet assignments remain future work.
- Removing a player or changing that player to spectator revokes their control; their sheets remain in the game.
- The user subsequently narrowed the first implementation to a game with multiple real network connections. Roles, sheet bindings, and persistent membership are not implemented in this slice.

## Implemented TCP Presence Slice

- `core/play/` is an independent C++20 backend module, built as `golarion_play`. It has no dependency on `core/sheet/`, JSON, a GUI toolkit, or a third-party networking library. `core/CMakeLists.txt` includes both modules; `core/play/` can also be built standalone.
- `Game` validates its ID/name and participant display names, assigns per-run participant IDs, maintains the currently connected roster, and returns detached `GameSnapshot` values. Different participants may have the same display name. Disconnect removes live presence; it does not implement persistent membership. A new connection currently receives a new identity.
- `GameHost` and `GameClient` use IPv4 TCP sockets with RAII ownership and polling APIs. The host handles multiple clients in one event loop, broadcasts complete presence snapshots after joins/departures, bounds message buffers, rejects malformed joins, and removes incomplete handshakes after five seconds. The initial transport caps concurrent sockets at 32 (or a lower platform descriptor limit), and send attempts have a one-second socket timeout; this is a basic synchronous polling transport, not a general production networking service.
- Configure the client's send timeout only after TCP connection succeeds. On Linux, `SO_SNDTIMEO` also limits `connect()` and may return `EINPROGRESS` (115 on the tested system) when a delayed connection exceeds one second; it is not evidence that the host accepted the connection. A loopback socket reproduction on Ubuntu/WSL confirmed the premature error with the old ordering and successful delayed connection with the corrected ordering. The native Windows multi-process test also passes after this fix; the C++ POSIX build remains unverified because the local Ubuntu environment has no C++ compiler.
- The private version-1 protocol carries a four-byte big-endian payload length, a version/type header, and length-prefixed text. It supports join, roster snapshot, and rejection messages, including fragmented/coalesced TCP input. The maximum payload is 64 KiB and individual text fields are at most 128 bytes. Protocol implementation details remain private to the module.
- `golarion_play_host` and `golarion_play_client` are diagnostic console executables for testing the backend, not the future frontend. Both require the address first and the port second: `golarion_play_host <IPv4 address> <port> [game name]` and `golarion_play_client <host address> <port> <participant name>`. A host process is not itself a participant, so run a client on that PC as well to join. The diagnostic game ID is currently `game`.
- This slice has no authentication, encryption, persisted game data, stable returning identities, roles, or sheets. It is a connectivity/presence prototype; protected Internet hosting remains part of the later design. No firewall/router configuration is modified by the tools.
- `play_game_test` and `play_protocol_test` cover the model and framing. `play_network_process_test` starts real C++ host/client processes on loopback and checks shared presence, fragmented input, malformed input, abrupt client loss, another connection, and host shutdown. The process test uses Python only as its test runner and is registered with CTest when a Python interpreter is available.
- Windows verification used GCC 10.3 with C++20 and warnings enabled: both C++ tests and the real process test passed. The sandbox blocked loopback connects with WSA error 10013; the process test passed when executed with local socket permissions. The user also confirmed a real Windows-host/Linux-client connection over Tailscale after the reachability check and scoped firewall-rule instructions. CMake is not installed in the current Windows environment, so the CMake configuration itself was not run; the full automated C++ POSIX suite remains unverified locally.

See `development.md` for standalone build and launch commands. The following sections describe later game-management behavior, not additional implemented features.

## Proposed Game Model and Lifecycle

Keep the persistent game and live connection state distinct:

- `Session`: the main aggregate, currently owning its function registry and RoleManager. Role assignments are keyed by caller-supplied stable user IDs. Generation/authentication of those identities, persisted membership/roles, name, sheet bindings, settings, and state remain future work. The prototype's `Game` remains only its connected-roster model.
- Hosting lifecycle: starting/stopping the server is a future responsibility of `Session`; persistent membership and session state must be designed separately from live connections.
- `Participant`: stable identity, display name, role, and access credentials for a game. Display names and `CharacterIdentity.playerName` are descriptive text, not authorization.
- `Presence`: current connection and synchronization status. Disconnecting does not remove membership, sheet assignments, or control permissions.
- `SheetBinding`: game-owned sheet ID, permitted controllers/readers, publication policy, and revision, wrapping the existing `CharacterSheet`.

Proposed flow: create or reopen a game, start hosting, invite/admit participants, prepare/import/assign sheets, play, save, and close hosting. Joining an existing game restores its participant rather than creating a new member on every connection. Preparation may begin with the creator alone; enforce any minimum connected master/player requirement only when an explicit live-play lifecycle actually needs it.

## Proposed Permissions

| Capability | Game master | Player | Spectator |
| --- | --- | --- | --- |
| View and modify game sheets | All game sheets | Assigned sheets only | No editing; published information only |
| Import a personal sheet | Yes | Yes; initially controlled by its importer | No |
| Control characters | Explicit assignments, including NPCs and delegated PCs | Explicit assignments; may include multiple sheets | No |
| Receive game state | All information authorized for masters | Own/assigned sheets and shared information | Published information only |
| Manage membership and roles | According to administrative policy | Own join/leave operations | Own join/leave operations |

Role and character control are separate: a master may also control a player character, and a player may control several characters. The hosting user has the distinct immutable `Host` role with master privileges. Several masters may operate concurrently against one authoritative game once roles are implemented.

Confirmed administrative policy: only the host may assign `Master` or change a master's role; a master may only change other players/spectators to `Player` or `Spectator`. The host role cannot be changed or granted to another participant by the role-change operation. Every future command must check current membership and role, so demotion or removal takes effect on subsequent requests. Shared player control is confirmed; exact spectator publication fields remain open.

## Proposed Host Authority and Sheet Integration

The hosting application runs the authoritative game service. All participants, including masters, submit commands through the same application-service boundary. The host checks identity, permissions, revisions, and supported operations before mutating the model, saving, and publishing recipient-specific results.

Serialize domain mutations through the game's command queue. This permits simultaneous masters without requiring concurrent mutation of `CharacterSheet`. Reuse existing sheet mutation APIs and their feat-prerequisite rechecks. Keep game membership and networking outside sheet rules and the sheet-specific `golarion::ResourceManager`.

Use stable game, participant, and sheet IDs in the protocol. C API handles and C++ addresses are process-local and never represent network identities. FFI is the existing local library boundary; network messages are a separate application boundary.

Proposed import semantics:

1. A player selects a sheet from their local application library.
2. The application submits its versioned source data through the import command.
3. The host validates supported save/rules/catalog versions, reconstructs the sheet, and assigns a new game sheet ID and control to the importing participant.
4. The game-owned copy becomes authoritative during play. Its accepted changes are saved with the game on the host.
5. The participant may export an authorized updated sheet back to their local library. Importing does not silently overwrite the original local sheet, and independent offline edits are not silently merged into the game copy.

These copy/import/export semantics are a recommendation rather than an already confirmed user choice. Masters have access to imported game copies, not unrelated sheets in another participant's local library.

## Required Inter-Application Features

| Area | Initial behavior to design |
| --- | --- |
| Hosting | Create/open/save a game; start/stop accepting connections on an address and port. |
| Connecting | Join by host address and port, directly on LAN/Internet or through a VPN; report connection failures. LAN discovery can follow basic manual connection. |
| Admission and identity | Game access credentials or invitations, optional admission by a master, stable returning identity, and access revocation. Exact credential UX is still open. |
| Compatibility | Negotiate protocol and sheet/rules/catalog compatibility before importing or applying incompatible operations. |
| Membership and presence | Retrieve participants and roles, distinguish offline/connecting/synchronizing/online, and propagate joins, disconnects, removals, and role changes. |
| Initial synchronization | Receive game metadata, permissions, visible sheet summaries, and authorized sheet state. |
| Sheet transfer | Import a locally created sheet, receive the canonical game ID, and export an authorized updated copy. |
| Sheet control | Assign/revoke control and visibility, and propagate permission changes. |
| Sheet interaction | Submit supported operations against a game sheet and receive an accepted/rejected result and updated authorized state. |
| Concurrency | Carry command IDs and sheet revisions; detect conflicts and reject stale conflicting edits instead of overwriting silently. |
| Recovery | Reconnect as the same participant and retrieve the current permitted state; deduplicate retryable commands. |
| Availability | Distinguish client disconnect from host shutdown. If the host becomes unavailable, live game operations stop until it returns; automatic host migration is outside the initial proposal. |
| Durability | Save accepted authoritative changes consistently before acknowledging them as durable; recover the game from an interrupted save. |

## Proposed Message Contract

The immediate protocol covers only session admission, connected-user state, role changes, and disconnection. JSON over TCP and permanent per-session user tokens are confirmed; [session-protocol.md](session-protocol.md) holds the proposed framing and message contract. The broad sheet-related examples below remain later proposals, not the initial protocol implementation.

Keep network contracts independent from the C++ memory layout and from internal persistence structures. The existing `SaveData` supports importing complete source data; the current `View` structs still require explicit wire serialization and permission filtering before they become network snapshots.

| Direction | Examples |
| --- | --- |
| Client to host | `JoinGame`, `ResumeConnection`, `ImportSheet`, `GetSheet`, `ApplySheetCommand`, `AssignSheetControl`, `ExportSheet`, `LeaveGame`. |
| Host response | `JoinedGame`, `GameSnapshot`, `SheetImported`, `SheetSnapshot`, `CommandAccepted`, `CommandRejected`, `RevisionConflict`. |
| Host event | `ParticipantPresenceChanged`, `ParticipantRoleChanged`, `SheetChanged`, `SheetPermissionsChanged`, `HostClosing`. |

Commands carry a protocol version, a unique request/command ID, and the relevant game/entity IDs. Sheet mutations also carry the expected sheet revision. Authenticate the sender through its accepted connection; a client-supplied participant ID never grants authority by itself. Events carry an authoritative sequence/revision so a reconnecting client can detect a gap and request a new snapshot.

For example, two masters editing revision 12 of one sheet do not silently replace one another: after the first accepted change produces revision 13, a conflicting second change receives the current revision and must be reviewed against refreshed state. Retrying the same accepted command ID returns the prior outcome instead of applying an effect twice.

Prefer complete authorized snapshots at join/reconnect and per-sheet refresh after a change for the first implementation. Incremental patches are an optimization, not a prerequisite. A client awaiting synchronization cannot present a local draft as accepted game state.

Authorization and filtering apply to snapshots, events, and exports before transmission. Spectators receive only published data; hiding fields in the UI is insufficient. Plan authenticated protected connections for direct Internet use; the exact TLS/credential setup belongs to transport design.

Initially use reliable ordered messages for commands and synchronization. Transport delivery alone does not solve application-level identity, permissions, retries, save durability, or reconnect state. No transport library has been selected. [Qt Network](https://doc.qt.io/qt-6/qtnetwork-programming.html) is one C++ candidate offering TCP/IP client/server and TLS facilities; the domain model must remain independently testable.

## Possible Implementation Slices

The sheet backend lives in `core/sheet/`; `core/play/` now implements the first TCP presence slice. A native C++ desktop frontend may use an application-level C++ API, while the existing C API remains a separate interoperability facility; the final frontend boundary is not selected yet.

1. Add ProtocolManager and the connected-user/identity subsystem incrementally, using the implemented RoleManager features through the Session-owned registry. Integrate server ownership in a separately agreed step.
2. Add the sheet ID/name registry, shared assignments, and authorization separately, then connect sheet contents through an application service.
3. Add persistent game storage, import/export, recipient-specific state, command IDs, revisions, and identity-preserving reconnect in separately reviewable steps.
4. Validate actual multi-PC LAN/Internet/VPN operation and introduce authenticated protected connections before treating the prototype as complete Internet hosting.

Useful acceptance scenarios: a player imports and controls their own sheet; another player cannot edit it; both masters can modify it; concurrent conflicting changes are explicit; a spectator receives only published state; reconnecting restores identity and control without duplicating an action; role revocation takes effect; and restarting the host restores the persistent game.

Remaining details for later design: invitation/credential UX, precise spectator views, exact persistence storage, any later transport-library adoption, and future UI toolkit. In-app human communication is outside this design.
