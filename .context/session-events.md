# Live Notifications

Status: permissions.changed implemented 2026-11-18; session.participantsChanged added 2026-11-19. Permission changes notify only the affected user; connected-participant snapshots notify admitted users without extra permission bits. See session-participants.md for the derived projection and revisions. Frontend presentation remains separate.

## Separate Command and Event Dispatch

ServerProtocol owns one commands_ registry per instance. ClientProtocol independently owns an event callback registry and dispatchEvent; neither protocol inherits from the other. Session forwards registration and sending without implementing transport/domain logic.

| Operation | Host | Client |
| --- | --- | --- |
| add_command(name, requirements, callback) | Registers an incoming request with current permission checks. | Not exposed. |
| on_event(name, callback) | Not exposed. | Registers an incoming notification from the admitted host. |
| dispatch (server) / dispatchEvent (client) | Validates the current actor/binding and required bits, then invokes the callback. | Validates the current admitted host connection, then invokes the callback without a local PermissionManager. |
| sendCommand(name, payload) | Queues execution as hostId through LocalCommands; returns future<json>. | Queues a correlated TCP request; returns future<json>. |
| sendEvent(userId, name, payload) | Queues an uncorrelated notification for the current binding; returns bool. | Not exposed. |

Hosts publish protocol.add_command (AddCommandSignature); clients publish protocol.on_event (OnEventSignature). Hosts additionally publish protocol.sendEvent using SendEventSignature; the identity commit service remains internal/host-only. Constructor rollback and destruction remove only successfully owned registrations.

Client CommandContext contains the current connection generation in connectionId and no userId: the sender is the connected host, whose participant ID is not part of admission. ClientConnection creates the context locally; payload fields cannot define it. Cached generations, missing contexts, participant claims and disconnected/admitting sources reject. Public synchronous dispatch remains a trusted in-process API, as on hosts.

Client event callbacks return void: no reply, promise or request ID is created. request.accepted and request.rejected are reserved client registration/event names. Built-in permissions.changed cannot be replaced by another registration.

## Framing and Routing

The codec is unchanged JSON v4: a four-byte big-endian size followed by strict UTF-8 JSON, maximum 65536 JSON bytes. It already permits omitted requestId. Both binaries must support the emitted built-in notifications; client builds lacking session.participantsChanged cannot receive the new extension.

```json
{"version":4,"type":"permissions.changed","payload":{"userId":"user-example","permissions":23}}
```

ClientConnection::processMessages decodes complete frames in stream order, including leftovers from admission. With requestId, a message must be a valid accepted/rejected reply matching a fully sent pending command. Without requestId, it goes through ClientProtocol::dispatchEvent and the client registry. A reply envelope without its ID, an unknown notification, invalid payload or throwing handler closes the connection and fails remaining promises; completed results/credentials remain completed.

Partial notifications are allowed while idle. Notifications may be fragmented or coalesced with replies, do not consume a command future, and do not renew its fixed five-second deadline. The host still requires requestId on every incoming request: clients cannot turn notifications into a permission-check bypass.

## Outbound Queue and Lifetime

sendEvent validates the target/name/payload and codec limits, resolves the current UserId binding under execution/connection synchronization, and queues encoded bytes under a separate events mutex. True means accepted for sending, never acknowledged delivery. False means no current binding, inactive/stopping transport, or a full queue. Malformed arguments throw.

Each connection has at most 64 queued event frames and 256 KiB of queued bytes, plus the transport's one bounded in-flight frame. Overflow marks that peer for disconnection and discards its queued events, allowing an explicit resume to resynchronize. The queue is keyed by ConnectionId rather than just UserId; replacement/close removes that generation's entries. There is no offline journal or replay.

Only Hosting writes server sockets. prepareOutput alternates queued notifications and buffered requests when both are available, preserving each frame and avoiding starvation. A join/resume reply occupies the output before subsequent events, so admission is decoded first. Select remains bounded to 50 ms.

sendEvent never takes hostingMutex_: stop may hold it while joining the worker that runs a command callback. stop disables/clears the outbox before joining; worker termination also clears it. Existing buffered-request stop checkpoints remain in place. Restart creates fresh bindings and re-enables the queue, preserving identity/mask/token state.

Client lifecycle ownership uses a shared ClientConnection plus an execution-mutex-protected weak active reference. Registration/dispatch/snapshot access use executionMutex_; the worker never takes clientMutex_, which lifecycle operations may hold while joining it. The connection is published before its worker starts. Client callbacks may inspect snapshots or perform nested dispatch; same-manager join/resume/disconnect/sendCommand calls reject before lifecycle locks to prevent joining or waiting on their own worker. Callbacks execute on the receiving thread and should finish promptly.

## Permission Transaction Hook

The permission command adapters read the previous target mask, run the existing identity.updateUserPermissions transaction, then call protocol.sendEvent only if the effective mask changed. Emission happens after disk replacement and PermissionManager's candidate-map swap, outside the permission mutex. Denied operations, failed writes and effective no-ops produce no notification. The command's ordinary correlated response still goes to its caller.

The built-in client callback validates exactly userId/permissions, requires its own admitted UserId, converts the numeric mask through the shared strict parser and updates ClientSnapshot under the connection state mutex. Zero remains a valid connected permission set. Only the affected user receives the event; administrators receive their command result and may query other users with permissions.get.

An offline target is updated persistently and receives its current mask through resume. Delivery failure cannot roll back an already committed transaction or convert it into a failed mutation. Direct trusted PermissionManager setters remain in-memory operations and do not automatically persist or emit events. No permission business rule is moved into Session or client-side authorization.

## Verification

play_events_test exercises registration/RM ownership, targeted delivery, common/nested dispatch, strict masks, zero snapshots, idle fragmented events, mixed/out-of-order replies, fixed deadlines, committed changes/no-ops/failed writes, offline resume, stale contexts, frame/byte queue limits, replacement, stop/restart and callback/lifecycle synchronization. Existing client tests now distinguish incomplete event prefixes from complete invalid frames. Build and CTest use the root CMakeLists.txt with output in build/core/play.

All seventeen play suites pass on Windows through the root Debug/Ninja build. POSIX and physical two-PC testing of this step remain unverified. See development.md and session-flows.md for commands and interaction diagrams.
