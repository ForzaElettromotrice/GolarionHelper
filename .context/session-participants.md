# Connected Participants

Status: implemented 2026-11-19. Only currently admitted, connected users are listed, plus the local host. Every admitted user may read the list and receive updates without any required permission bit. Pending peers cannot read it. Offline identities/tokens remain stored but are not advertised.

## Data and Authority

ParticipantInfo in golarion/play/participants.hpp contains UserId and isHost. It is a detached public projection, not a second user/connection registry. Native sockets, transport ConnectionIds, token hashes and participant permissions remain in their existing owners. Resume can change ConnectionId while retaining the same ParticipantInfo. The host is represented locally without a fabricated socket.

ParticipantsSnapshot contains a uint64 revision and vector<ParticipantInfo>. ProtocolManager::getParticipants derives it from current matching activeConnections_/connections_ bindings and permissions.getHostId, under execution/connection synchronization. The host is first; other users are sorted by UserId. Duplicate host bindings do not add a second host entry.

Only the revision counter is stored on the host; no parallel participant list is maintained. It advances for successful new bindings and removal of active bindings. Pending connections, failed admission, permission changes and replacement of an already-connected identity do not advance it. The revision is runtime state, not saved in archives. Restoring identities alone restores no presence.

## Command and Event

Host registerParticipantCommands registers session.getParticipants through the existing add_command path with CommandRequirements{}: authenticated, no required bits. The exact request payload is an empty object. Both host and clients can use Session::sendCommand; the host local worker also supports offline queries. There is no extra ResourceManager feature or dispatcher for presence; permissions.getHostId and existing command/event services are reused.

The result and session.participantsChanged event share this payload:

```json
{"revision":2,"participants":[{"userId":"host","isHost":true},{"userId":"user-example","isHost":false}]}
```

The event omits requestId and uses sendEvent. A command result retains the ordinary request.accepted/result envelope and its requestId. Explicit queries return JSON; live client cache updates are driven by the event handler, not by arbitrary command results.

## Lifecycle Hooks

- admitNewUser publishes only after registration/token persistence succeeds and identity/connection locks are released. Failed persistence rolls back the staged binding and emits nothing.
- bindUser publishes a new list for a newly connected identity. When replacing an existing connection it clears the old generation's event queue and sends only the initial snapshot to the replacement; other clients keep the same list/revision.
- closeConnection publishes only if it actually removes the current active binding. Pending sockets and late closes of replaced generations emit nothing.
- publishParticipantsChanged increments the revision, builds one snapshot and queues it for admitted users using sendEvent. sendParticipantsSnapshot sends the current revision to one user. No connection lock is held while calling sendEvent, and neither helper takes hostingMutex_. Delivery cannot undo a committed binding.
- A snapshot serialization/frame-limit failure marks affected event queues for closure, allowing the existing worker to disconnect them instead of retaining stale presence. Queue saturation keeps the same policy. The revision counter never wraps; exhaustion closes affected peers as a delivery failure.
- Hosting's existing one-frame output ensures the join/resume response precedes the new connection's initial snapshot. Other clients may receive their updates independently. Codec and admission result schemas remain JSON v3, without new fields.
- Stop disables the event queue before joining and clears all bindings. Clients clear their cached participants with the rest of ClientSnapshot on disconnect. Identity archives and tokens are preserved; a new resume receives a fresh snapshot.

## Client and Views

registerClientCommands registers session.participantsChanged with the existing callback-only add_command overload. dispatch checks the current admitted host connection before parsing/updating. ClientConnection::updateParticipants uses its state mutex; it never acquires the lifecycle clientMutex_.

The parser requires exactly revision/participants, an integer nonnegative uint64 revision, a nonempty array, exact userId/isHost entry fields, valid unique IDs, boolean host flags and exactly one host. Applied snapshots must include the client's admitted UserId. Older revisions are ignored; identical duplicates are idempotent; conflicting equal revisions or a changed host identity within one connection reject. Invalid notifications close the connection through the existing worker error path.

ClientSnapshot.participants is optional until the first complete event. It is copied out through getClientState and SessionView.client and clears on disconnect. HostSessionView.participants is built directly from the bindings. The [desktop participant panel](frontend.md#desktop-application) consumes both snapshots through Session::toView(). Presence is transient.

All snapshot updates retain the existing frame, event-queue and stop constraints documented in session-events.md. Each fresh ClientConnection starts with no cached revision, so a lower revision after host reconstruction is valid. Both binaries must understand the new built-in event; no event subscription or compatibility negotiation is introduced.

## Verification

play_participants_test covers strict schemas, empty/zero/max revisions, offline-user filtering, local host projection, pending access rejection, initial snapshots, zero-permission access, failed persistence, replacement without duplicate presence, late disconnect, offline resume, stop, host reconstruction, detached Views, fragmented/coalesced events, stale/conflicting revisions and simultaneous admissions/departures. Admission/event regression helpers explicitly handle the newly expected presence notifications while checking correlated responses and shutdown. Build/tests use the repository-root CMakeLists.txt and build/core/play outputs.

All seventeen play suites pass on Windows through the root Debug/Ninja CMake build. The final run also verifies disconnection when a snapshot exceeds the codec's frame limit. POSIX and physical two-PC validation of this step remain pending.
