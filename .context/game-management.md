# Game Management and Application Communication

Status: Session/manager backend, TCP hosting, identity persistence, JSON admission, and authenticated registered-command dispatch implemented, 2026-11-12. SessionCode candidates are generated/verified. Game/network and the old client/tests are removed. Client-side ProtocolManager, concrete role/sheet commands, user-list events, and display names remain deferred.

## Confirmed Requirements

- A native desktop application running on PCs, developed in C++.
- A persistent game supports one or more game masters, one or more players, and optional spectators; it can be reopened across multiple play sessions.
- Participants connect from separate PCs over LAN or the Internet.
- A participant's PC hosts the game. The game is reachable while the hosting application is open; its saved data survives closing the application.
- Direct Internet access requiring network configuration, or a VPN, is acceptable for the initial version. A managed online relay or always-on dedicated server is not required.
- Players may import character sheets created locally in the application and modify sheets assigned to them. Masters may modify all game sheets.
- Communication currently means inter-application networking for joining and interacting with a shared game. In-app chat, voice, and video are outside the current scope.
- Implementation proceeds in small backend-only steps. Session, its registry/managers, and pending TCP hosting are implemented; the frontend and client-side ProtocolManager remain deferred.
- The implemented rules engine remains Pathfinder 1e. The user's general reference to DnD does not select another ruleset.

## Confirmed Structural Decisions for Later Steps

- `Session` is the main play container, analogous to `CharacterSheet`. A user creates a session and can start/stop its server; subsystem behavior is delegated to its managers.
- `Session` requires a host UserId and owns ResourceManager, ProtocolManager, and RoleManager in that order. It delegates hosting, connections, user registration, token issuance, and identity save/load. ProtocolManager handles admission and authenticated dispatch internally; concrete role/sheet command providers remain future work.
- Session is a container/facade: ProtocolManager directly implements logical registration, binding/replacement, lookup, context construction, and close. Session forwards these calls except `getUserId`, which remains only on ProtocolManager; no connection features are published through ResourceManager. Binding trusts an identity verified by its caller. ProtocolManager owns private credentials and admission handlers; no separate ConnectionManager is introduced. RoleManager owns roles; see `session-protocol.md`, `session-tokens.md`, and `session-admission.md`.
- RoleManager publishes registration, lookup, permissions, snapshots, and staged registration through its seven `roles.*` features. It synchronizes data and owns the immutable Host/default Players; snapshot/commit APIs are trusted local functions. ProtocolManager publishes only `protocol.add_command` and reuses dispatch for authenticated local/remote calls. See `session-roles.md` and `session-commands.md`.
- Session is the local hosting entry point: start/stop delegates to ProtocolManager's private Hosting implementation, without polling. Pending peers join/resume via JSON; admitted sockets execute registered commands and remain open after ordinary errors. Destruction stops hosting before provider teardown. Lifecycle operations belong outside command callbacks; see `session-admission.md` and `session-commands.md`.
- A session has a generated five-character SessionCode candidate using `A-Z` and `0-9`; local verification accepts lowercase by normalization. ProtocolManager owns the value and Session delegates its display getter. It will accompany IP/port during admission. The user explicitly deferred the shared host registry and persistence: uniqueness among sessions of one host remains a requirement, not a current guarantee.
- The hosting user has the unique, immutable `Host` role with master privileges. A new incoming user starts as `Player`. Only the host may promote a user to `Master` or change an existing master's role. Masters may change other players/spectators between `Player` and `Spectator`; they cannot modify another master or the host. Players and spectators cannot change roles.
- `Role::Any` is an access threshold below Spectator, never assignable or persisted. Live pending connections have no UserId and satisfy only Any; unknown/stale connections cannot execute. ProtocolManager registers join/resume through `add_command` with Any before exposing the feature, using the same lookup/normalization/role check as every command. All successes use `request.accepted` with `payload.result`; join returns UserId/Player/token, resume returns UserId/current role.
- Communication will use JSON over TCP. The host issues a permanent personal token at the first admission of a new client; it identifies that user within the same `Session`, including after session restarts, and preserves their current assigned role on resume. The proposed message contract and remaining protocol details are in [session-protocol.md](session-protocol.md).
- Token issuance/recognition, identity archives, and JSON join/resume are implemented: eight random bytes, OpenSSL Crypto, host-side hashes only, no SessionId, and only the original token required on the client. Session delegates save/load; later role changes need an explicit save. No display names are stored. Client storage remains separate; see `session-tokens.md` and `session-admission.md`.
- A valid resume replaces any previous live connection for that identity, preserving the permanent token and current role. There is one active connection and one connected-user entry per identity; stale messages/disconnects from the replaced connection cannot affect the replacement.
- Store sheet IDs and names in the game registry; connect actual `CharacterSheet` content through the later application service.
- A sheet may have several authorized players; masters retain access to all game sheets.
- All masters can administer the permitted player/spectator role changes; master promotion/demotion is reserved to the immutable host. Previously discussed shared-sheet assignments remain future work.
- Removing a player or changing that player to spectator revokes their control; their sheets remain in the game.
- The current network slice manages pending and admitted TCP connections. Identity/role persistence and token-based resume are implemented; connected-user synchronization, role commands, and sheet bindings remain separate steps.

## Implemented Session Hosting

- `core/play/` is an independent C++20 module built as `golarion_play`, with nlohmann_json 3.12, OpenSSL Crypto 3.0+, standard threads, and native sockets. It has no sheet/frontend dependency and can be built standalone.
- Session contains ResourceManager, ProtocolManager, and RoleManager. Its hosting and connection operations delegate to ProtocolManager; that manager owns logical IDs/bindings, a private Hosting implementation, and the network thread.
- `golarion_play_host <IPv4 address> <port> [identity archive]` creates `Session("host")`, creates/loads the archive (default `session-identities.json`), starts hosting, prints port/code, and waits for Ctrl+C. Port 0 requests an OS-assigned port. Its main thread does not poll networking.
- Accepted sockets start pending; join/resume verifies code/token, creates a Player or replaces the previous connection, and keeps admitted sockets open. Generic Any commands can run while pending without admitting the peer or prolonging its five-second deadline. Pending errors close the socket; ordinary admitted errors keep it open. The codec retains its four-byte prefix/64 KiB cap. Every request executes a registered callback after common role checks and receives a correlated result/rejection. See `session-admission.md` and `session-commands.md`.
- Connection/lifecycle state is synchronized. Stop joins the worker and releases sockets and bindings before provider teardown, preserving roles and the ID sequence. See `protocol-manager.md` for lifecycle details.
- Client-side ProtocolManager and a replacement console client remain to be implemented. Game/network and their client/model/process tests are removed; useful earlier details are archived in `history/2026-11-10-game-network-prototype.md`.
- Eleven native suites cover admission, SessionCode, UserToken, the private token registry, identity persistence, hosting, codec, Session, ProtocolManager, RoleManager, and ResourceManager. A real host-process restart smoke test verifies JSON resume with the original client token. Windows verification uses GCC C++20 with strict warnings. Local TCP execution requires socket permissions because the sandbox blocks connects with WSA error 10013; CMake and the POSIX branches remain unverified locally. See `development.md`.

## Proposed Game Model and Lifecycle

Keep the persistent game and live connection state distinct:

- `Session`: owns its registry and managers. Local user registration accepts stable IDs; network admission generates IDs, persists credentials/roles, and builds trusted context. Generic authenticated dispatch is implemented. Session name, sheet bindings, settings, other game state, and concrete role/sheet commands remain future work.
- Hosting lifecycle: starting/stopping the server is implemented through Session delegations to ProtocolManager; persistent membership and session state must be designed separately from live connections.
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

The codec, Session hosting, admission, and authenticated registered-command dispatch are implemented; `session-protocol.md` separates current behavior from later messages. Concrete role commands and user-list synchronization remain pending. Sheet-related examples below remain proposals.

Keep network contracts independent from the C++ memory layout and from internal persistence structures. The existing `SaveData` supports importing complete source data; the current `View` structs still require explicit wire serialization and permission filtering before they become network snapshots.

| Direction | Examples |
| --- | --- |
| Client to host | Implemented `session.join`/`session.resume`; planned `session.leave`/`users.setRole` and sheet operations. |
| Host response | Implemented minimal `session.joined`, `session.resumed`, and correlated `request.accepted`/`request.rejected`; planned user snapshots and sheet events. |
| Host event | `ParticipantPresenceChanged`, `ParticipantRoleChanged`, `SheetChanged`, `SheetPermissionsChanged`, `HostClosing`. |

Commands carry a protocol version, a unique request/command ID, and the relevant game/entity IDs. Sheet mutations also carry the expected sheet revision. Authenticate the sender through its accepted connection; a client-supplied participant ID never grants authority by itself. Events carry an authoritative sequence/revision so a reconnecting client can detect a gap and request a new snapshot.

For example, two masters editing revision 12 of one sheet do not silently replace one another: after the first accepted change produces revision 13, a conflicting second change receives the current revision and must be reviewed against refreshed state. Retrying the same accepted command ID returns the prior outcome instead of applying an effect twice.

Prefer complete authorized snapshots at join/reconnect and per-sheet refresh after a change for the first implementation. Incremental patches are an optimization, not a prerequisite. A client awaiting synchronization cannot present a local draft as accepted game state.

Authorization and filtering apply to snapshots, events, and exports before transmission. Spectators receive only published data; hiding fields in the UI is insufficient. Plan authenticated protected connections for direct Internet use; the exact TLS/credential setup belongs to transport design.

Initially use reliable ordered messages for commands and synchronization. Transport delivery alone does not solve application-level identity, permissions, retries, save durability, or reconnect state. No transport library has been selected. [Qt Network](https://doc.qt.io/qt-6/qtnetwork-programming.html) is one C++ candidate offering TCP/IP client/server and TLS facilities; the domain model must remain independently testable.

## Possible Implementation Slices

The sheet backend lives in `core/sheet/`; `core/play/` implements the Session/manager backend and pending TCP hosting. A native C++ desktop frontend may use an application-level C++ API, while the existing C API remains a separate interoperability facility; the final frontend boundary is not selected yet.

1. Register concrete role commands through existing authenticated dispatch, implement client-side ProtocolManager/token retention, then user-list events. Role data and local/remote dispatch are synchronized; preserve provider lifetime and callback constraints in `session-commands.md`.
2. Add the sheet ID/name registry, shared assignments, and authorization separately, then connect sheet contents through an application service.
3. Add persistent game storage, import/export, recipient-specific state, command IDs, revisions, and identity-preserving reconnect in separately reviewable steps.
4. Validate actual multi-PC LAN/Internet/VPN operation and introduce authenticated protected connections before treating Session hosting as complete Internet support.

Useful acceptance scenarios: a player imports and controls their own sheet; another player cannot edit it; both masters can modify it; concurrent conflicting changes are explicit; a spectator receives only published state; reconnecting restores identity and control without duplicating an action; role revocation takes effect; and restarting the host restores the persistent game.

Remaining details for later design: invitation/credential UX, precise spectator views, full game storage beyond the identity archive, any later transport-library adoption, and future UI toolkit. In-app human communication is outside this design.
