# Game Management and Application Communication

The desktop app creates local campaigns, hosts and joins them with identity-preserving resume. Connected-participant snapshots carry names using JSON v4. The host reads archive v2 and saves archive v3 with per-user display names. Sheet commands remain future work.

## Confirmed Requirements

- A native desktop application running on PCs, developed in C++.
- A persistent game supports one or more game masters, one or more players, and optional spectators; it can be reopened across multiple play sessions.
- Participants connect from separate PCs over LAN or the Internet.
- A participant's PC hosts the game. The game is reachable while the hosting application is open; its saved data survives closing the application.
- Direct Internet access requiring network configuration, or a VPN, is acceptable for the initial version. A managed online relay or always-on dedicated server is not required.
- Participants may import sheets created locally in the application and interact with assigned/shared sheets according to explicit permissions and bindings. Master/Player/Spectator are configurable permission presets; their names alone must not authorize sheet operations. Concrete sheet permissions remain undefined behind the current placeholder flags.
- Communication currently means inter-application networking for joining and interacting with a shared game. In-app chat, voice, and video are outside the current scope.
- Implementation proceeds in small steps. Session, managers, admission/sendCommand, both console tools, permission RPCs, targeted permission notifications and connected-user query/events are implemented. The desktop app exposes the host and admission lifecycles. Participant-list presentation and sheet commands are separate steps.
- The implemented rules engine remains Pathfinder 1e. The user's general reference to DnD does not select another ruleset.

## Confirmed Structural Decisions for Later Steps

- `Session` is the main play container, analogous to `CharacterSheet`. A user creates a session and can start/stop its server; subsystem behavior is delegated to its managers.
- Session::createHost(hostId) takes the immutable Host UserId; createClient has no authoritative user manager. Both own ResourceManager/ProtocolManager, and only hosts own PermissionManager. Methods delegate; see session-client.md.
- Session is a facade: ProtocolManager implements logical binding/context/lifecycle, credentials and admission. No ConnectionManager is introduced. PermissionManager owns all users/masks and is consumed through ResourceManager. UserId is shared in user_id.hpp.
- Host PermissionManager registers permission command adapters through protocol.add_command. ProtocolManager exposes identity.updateUserPermissions and protocol.sendEvent; clients register host-notification callbacks through their own protocol.add_command feature. ClientSnapshot refreshes on permissions.changed after effective committed changes. See session-events.md.
- Session is the local hosting entry point: start/stop delegates to ProtocolManager's private Hosting implementation, without polling. Pending peers join/resume via JSON; admitted sockets execute registered commands and remain open after ordinary errors. Destruction stops hosting before provider teardown. Lifecycle operations belong outside command callbacks; see `session-admission.md` and `session-commands.md`.
- A session uses a five-character SessionCode with `A-Z` and `0-9`. Verification accepts lowercase by normalization. Desktop hosts supply the campaign's saved code to Session and ProtocolManager. Host-wide uniqueness remains deferred.
- The implemented model stores immutable Host UserId separately from flags. Host owns all six permissions. New users receive Player flags. Presets have no hierarchy; host alone grants/revokes ChangePermissions, delegates change only other non-host users, and presets replace ordinary bits while preserving that flag.
- Permission::Any = 0 explicitly selects public command access for registered join/resume; it is not assignable. A zero PermissionSet requirement instead needs an authenticated registered user. Unknown/stale contexts reject, and identity never comes from payload. All requests reuse the common dispatcher.
- Communication uses framed JSON v4 over TCP. Original eight-byte credentials identify participants within their archive across reconnect/restart; host resolves their current permissions. Both peers must use the new wire version.
- Token issuance/recognition uses OpenSSL Crypto with host-side hashes only and no SessionId. Version-2 archives store explicit hostId, users/masks and hashes. Role archives are rejected without conversion per user choice; no automatic deletion occurs. Permission commands save transactionally; trusted direct setters still require an explicit save.
- Valid resume replaces a previous live connection while preserving identity/token and returning the current mask. One active binding exists per user; stale messages/disconnects cannot affect its replacement.
- Store sheet IDs and names in the game registry; connect actual `CharacterSheet` content through the later application service.
- A sheet may have several authorized participants. Shared-sheet assignments and concrete read/write permission semantics remain future work; do not infer them from preset names.
- Permission and sheet-control revocation must affect later operations without deleting the participant's sheets. Preset-to-sheet effects remain undefined until concrete sheet permissions are selected.
- Networking handles admission, generic commands, permission queries/mutations, transactional identity saving, token resume, targeted permission events and connected-only presence snapshots. The local host is included; all admitted users receive the list without extra permission bits. Sheet bindings remain separate work; see session-participants.md.

## Implemented Session Hosting

- `core/play/` is an independent C++20 module built as `golarion_play`, with nlohmann_json 3.12, OpenSSL Crypto 3.0+, standard threads, and native sockets. It has no sheet/frontend dependency and can be built standalone.
- Both Session modes own ResourceManager/ProtocolManager; only hosts initialize optional PermissionManager. All workers are stopped/joined before provider teardown.
- golarion_play_host uses createHost, loads/creates the archive and waits for Ctrl+C. It can expose an optional session name to admitted clients. golarion_play_client uses createClient and joins/resumes with its original token. Both workers progress networking automatically. See [development](development.md#host-and-client-tools) for syntax.
- Accepted sockets start pending; registered Any join/resume callbacks verify code/token, persist/register Player flags or restore current masks, and retain admitted peers. Successful generic public commands do not extend pending expiry. Ordinary admitted errors keep sockets open; framing/pending failures close them.
- Connection/lifecycle state is synchronized. Stop joins the network worker and releases peers/bindings while retaining permissions, tokens and the ID counter. Local commands remain available; shutdown joins them before provider destruction.
- Native admission and host/client sendCommand are implemented. The host queues local dispatch even offline; an admitted client sends correlated requests with bounded completion. User events and reconnect policy remain separate. Legacy Game/network and their old tests/client were removed; the current console still exposes admission/monitoring only, documented in `session-client.md`.
- Seventeen native play suites pass through the root CMake build on Windows, including RPC transactions, targeted live events, mixed replies, queue limits, full host reconstruction and prior stop/identity regressions. Outputs are build/core/play; POSIX remains unverified.

## Desktop Hosted Campaigns

The desktop controller saves each owned campaign under its stable UUID in the local `campaigns/` directory, with a name and a generated invitation code in `campaign.json`. Avvia hosting loads the code and loads or initializes `identities.json` in the same directory using the UUID as the host UserId. A sibling lock serializes hosting and moving the full campaign directory to the system trash. Hosting also takes the original in-directory lock for compatibility.

The Qt controller runs archive locking, session preparation, opening the IPv4 listener and collecting invitation addresses in a background task. It publishes the completed result on the GUI thread. The host uses the requested port and reports the actual bound port. The campaign name is published through `Session::registerSessionInfo`. Opening the campaign displays the exact saved code. For an older manifest missing a code, opening the page saves one atomically under the hosting lock before showing it. Participants keep their original credential and update the code once in that case.

`core/play` supplies typed hosted and joined campaign views and data. The joined view supplies name, address and code. Its data also contains the stored credential. `Session::toView()` composes live host or client state. Host-only `Session::toData()` combines the code with the identity snapshot from the protocol manager. Identity saves use the same identity projection under the archive lock. Qt adapts the views to QML and writes campaign data using the existing JSON keys.

The controller verifies the hosted campaign's confirmed ID, name and code, checks the on-disk manifest and acquires the campaign locks before moving it. Removing a joined record verifies the stored token against the selected local entry before deleting its file. On failure the controller retains the list entry. The hosted action uses the platform trash operation from [QFile](https://doc.qt.io/qt-6/qfile.html#moveToTrash).

## Proposed Game Model and Lifecycle

Keep the persistent game and live connection state distinct:

- Session owns its registry/managers and asks PermissionManager to register commands during host setup. Admission and permission mutations persist through the existing identity transaction pattern. Campaign names and identities have desktop storage. Other settings, sheet commands/bindings and full game persistence remain deferred.
- Starting and stopping a saved campaign reuses Session delegations to ProtocolManager. Persistent membership follows the identity archive independently of live connections.
- Participant: stable identity, effective permission mask and credentials. Display names remain deferred and descriptive. Client state contains its own admitted identity and latest mask received through permissions.changed.
- `Presence`: current connection and synchronization status. Disconnecting does not remove membership, sheet assignments, or control permissions.
- `SheetBinding`: game-owned sheet ID, permitted controllers/readers, publication policy, and revision, wrapping the existing `CharacterSheet`.

Proposed flow: create or reopen a game, start hosting, invite/admit participants, prepare/import/assign sheets, play, save, and close hosting. Joining an existing game restores its participant rather than creating a new member on every connection. Preparation may begin with the creator alone; enforce any minimum connected master/player requirement only when an explicit live-play lifecycle actually needs it.

## Permissions

Current decisions and the integrated model are in session-permissions.md. Permission1 through Permission5 are placeholders; ChangePermissions is the protected delegation flag. Sheet-specific semantics require future concrete permissions plus ownership/assignment checks.

The former role-based proposal is archived in history/2026-11-17-role-authorization.md; the removed manager API is in history/2026-11-17-role-manager.md. Neither is current authorization policy.

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
| Membership and presence | Connected-only listing and presence events are implemented; persisted identities and restricted permission queries remain separate. Richer connection states and removal commands remain future work. |
| Initial synchronization | Receive game metadata, permissions, visible sheet summaries, and authorized sheet state. |
| Sheet transfer | Import a locally created sheet, receive the canonical game ID, and export an authorized updated copy. |
| Sheet control | Assign/revoke control and visibility, and propagate permission changes. |
| Sheet interaction | Submit supported operations against a game sheet and receive an accepted/rejected result and updated authorized state. |
| Concurrency | Carry command IDs and sheet revisions; detect conflicts and reject stale conflicting edits instead of overwriting silently. |
| Recovery | Reconnect as the same participant and retrieve the current permitted state; deduplicate retryable commands. |
| Availability | Distinguish client disconnect from host shutdown. If the host becomes unavailable, live game operations stop until it returns; automatic host migration is outside the initial proposal. |
| Durability | Save accepted authoritative changes consistently before acknowledging them as durable; recover the game from an interrupted save. |

## Proposed Message Contract

Codec, hosting, admission, permission commands, targeted permissions.changed and session.getParticipants/session.participantsChanged are implemented. Sheet-related messages below remain proposals.

Keep network contracts independent from the C++ memory layout and from internal persistence structures. The existing `SaveData` supports importing complete source data; the current `View` structs still require explicit wire serialization and permission filtering before they become network snapshots.

| Direction | Examples |
| --- | --- |
| Client to host | Implemented session.join/session.resume, generic commands and permission adapters; planned session.leave and sheet operations. |
| Host response | Implemented correlated request.accepted/request.rejected, including admission; planned user snapshots. |
| Host event | Implemented permissions.changed for its affected user and session.participantsChanged for all admitted users; planned sheet, assignment and host-closing notifications. |

Commands carry a protocol version, request ID and operation-specific payload. Authenticate the sender through its accepted connection; a client-supplied participant ID never grants authority. Current permission events omit requestId, contain the full numeric mask and have no sequence/replay log; resume supplies the current mask. Sheet revision checks and event gap recovery are future contracts.

For future sheet editing, two masters editing revision 12 should not silently replace one another: after an accepted change produces revision 13, a conflicting change should receive current state for review. Idempotent retry by command ID is also future work, not current sendCommand behavior.

Prefer complete authorized snapshots at join/reconnect and per-sheet refresh after a change for the first implementation. Incremental patches are an optimization, not a prerequisite. A client awaiting synchronization cannot present a local draft as accepted game state.

Authorization and filtering apply to snapshots, events, and exports before transmission. Spectators receive only published data; hiding fields in the UI is insufficient. Plan authenticated protected connections for direct Internet use; the exact TLS/credential setup belongs to transport design.

Initially use reliable ordered messages for commands and synchronization. Transport delivery alone does not solve application-level identity, permissions, retries, save durability, or reconnect state. No transport library has been selected. [Qt Network](https://doc.qt.io/qt-6/qtnetwork-programming.html) is one C++ candidate offering TCP/IP client/server and TLS facilities; the domain model must remain independently testable.

## Possible Implementation Slices

The sheet backend lives in core/sheet; core/play implements host/client Session modes, TCP hosting, native client admission and managers. A future native desktop frontend can use the C++ application API; the existing C API remains separate and its full frontend boundary is undecided.

1. Review implemented connected-participant snapshots and choose a separate heartbeat/reconnection step, preserving RPC autosave, current actor checks, provider lifetime, revisions and buffered-stop constraints.
2. Add the sheet ID/name registry, shared assignments, and authorization separately, then connect sheet contents through an application service.
3. Add persistent game storage, import/export, recipient-specific state, command IDs, revisions, and identity-preserving reconnect in separately reviewable steps.
4. Validate actual multi-PC LAN/Internet/VPN operation and introduce authenticated protected connections before treating Session hosting as complete Internet support.

Useful acceptance scenarios: a player imports and controls their own sheet; another player cannot edit it; both masters can modify it; concurrent conflicting changes are explicit; a spectator receives only published state; reconnecting restores identity and control without duplicating an action; permission revocation takes effect; and restarting the host restores the persistent game.

Remaining details for later design: invitation/credential UX, precise spectator views, full game storage beyond the identity archive, any later transport-library adoption, and future UI toolkit. In-app human communication is outside this design.
