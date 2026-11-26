# Client Sessions and Commands

Status: connected-participant snapshots/query/events integrated 2026-11-19 alongside targeted permission events. Commands use sendCommand and notification handlers use on_event and use client-only dispatchEvent. Automatic reconnect remains separate; see session-participants.md and session-events.md.

## Modes and API

SessionServer is the host-only API. SessionClient() constructs the client with only ResourceManager and ClientProtocol. Its toView() returns a detached ClientSnapshot directly. Host operations are absent from the client API.

| Session/protocol API | Contract |
| --- | --- |
| joinSession(ip, port, SessionCode) | Owns arguments, starts a worker, connects to numeric IPv4/nonzero port and submits session.join; returns future<ClientAdmission>. |
| resumeSession(ip, port, SessionCode, UserToken) | Same transport with existing credential; returns current identity/mask without a new token or new-user fallback. |
| disconnectFromHost() | Cancels and joins the worker; idempotent while disconnected. |
| getClientState() const | Returns a synchronized detached ClientSnapshot, without credentials. |
| sendCommand(name, json) | Client requires completed admission and queues a correlated command; returns future<json>. Host queues trusted local dispatch, including offline. |
| on_event(name, callback) | Client registers an incoming host notification through the client event registry. Host registers commands separately. |
| sendEvent(userId, name, json) | Host queues a targeted notification without a response future. Not exposed by the client facade. |

ClientAdmission has UserId, PermissionSet permissions and optional UserToken. Only join returns the original credential. ClientSnapshot has state, optional UserId and optional PermissionSet; both optionals clear on disconnect. An optional containing mask zero is an admitted user with no permissions. Its mask starts from admission and refreshes on validated permissions.changed events; the server always authorizes from current state independently.

ClientSnapshot also contains optional ParticipantsSnapshot participants, filled by session.participantsChanged after the ordinary admission reply. It contains only connected users plus the local host, with UserId/isHost fields and a revision. SessionClient::getClientState() and toView() return detached copies; disconnect clears the cache. HostSessionView derives its participants directly from bindings. Queries use sendCommand("session.getParticipants", {}) with no required permission bits after admission. Offline identities, token data and other users' permission masks are not part of this list. See session-participants.md for strict validation and revision rules.

Connection states remain Disconnected, Connecting, Admitting and Connected. Mode/address/port misuse and overlapping active admissions reject synchronously; network/reply failures complete futures exceptionally. A disconnected/failed client can start another attempt after its old worker joins. Wrong-mode methods throw logic_error; noexcept code verification returns false on clients.

## Transport and Errors

Private ClientConnection owns the socket, input/output buffers, pending promises, state mutex, stop flag and worker. ClientProtocol's connection mutex serializes lifecycle/request IDs; the worker never acquires it. A weak active reference guarded by executionMutex_ supports dispatch and snapshot reads without taking a lifecycle lock that may be joining this worker. The connection is published before its thread starts. Only the worker touches the socket. Shared socket_support.hpp supplies RAII, numeric address validation, nonblocking platform errors and SIGPIPE handling.

Connect recognizes pending errors and checks SO_ERROR, with a five-second deadline. Admission send/receive share another five-second deadline. Select waits at most 50 ms for cancellation. The JSON v4 codec validates frame size, UTF-8, duplicate keys, nesting and envelope schemas. IDs client-N increase across attempts/commands. Admission must match its ID exactly, use request.accepted/result or a known request.rejected code, and contain exact UserId/permissions fields plus token only for join.

Permission masks must be nonnegative integers containing only known bits. Strings (including retired role names), booleans, floats, negative/unknown-bit values, missing masks and extra fields reject. Generic results can be any JSON value. Command replies match fully sent outstanding IDs; invalid/unknown/duplicate IDs and malformed schemas close the connection. Notifications omit requestId and pass to the client event dispatcher. Unknown notifications, malformed payloads or failing handlers also close the connection and fail remaining promises. Well-formed rejections complete only their request. Out-of-order replies and fragmented/coalesced notifications are supported.

At most 64 client commands are pending. Their five-second deadlines begin at submission, covering queued transmission and reply without renewal on partial progress. Timeout closes the connection and fails all pending requests. Disconnect/destruction cancels promises; EOF/reset reports a transport error. There is no replay or idle heartbeat timeout. A command may already have executed when its reply is lost.

Complete admission/command replies are resolved before following frames, independently of TCP grouping. A malformed later frame cannot invalidate a previously completed result or original token. Input survives admission for the normal command stage.


## Desktop Campaigns

The Qt controller uses the asynchronous Session client API and polls its futures and connection snapshot on the Qt event loop. A saved entry resumes with its original token, even when the user supplies a new address or code for that same stored campaign. Reusing an exact saved address and code via + Unisciti also resumes. Resume rejections stay visible and do not fall back to join. Only one connection is active per app process.

Each admitted campaign is saved atomically in an individual JSON file under the application's per-user local data directory (`campaigns/`). Token values stay in C++ and files use owner read/write permissions. Disconnection preserves these records; a malformed entry stays on disk. Admission and the host archive remain authoritative for membership.

Desktop hosts use `SessionServer::registerSessionInfo` for the campaign name. The desktop client requests it after admission. If the command is absent, the client uses the known campaign name or `Sessione <IPv4>:<port>`. Participant names are separate, supplied from the locally stored home field during v4 join/resume and published in participant snapshots. Host identities are saved in archive v4.

## Verification

The client suite retains fourteen admission/lifecycle/streaming scenarios, eight simultaneous clients through three host reconstructions, cancellation races, exact maximum frames and fixed deadlines. Existing sendCommand/permission-command coverage remains intact. All seventeen play suites pass through CMake/CTest from the root configuration on Windows; play_events_test adds idle fragmented notifications, mixed/out-of-order replies, live masks, strict schemas, stale generations and callback/disconnect safety. The separate permission_process_smoke.py is earlier permission-command verification. Diagnostics in build/play/client-validation describe retired JSON v2 and are historical. POSIX/physical two-PC verification of this step remains pending.
