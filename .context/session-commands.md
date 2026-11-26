# JSON Command Dispatch

ServerProtocol owns command registration and dispatch (including join/resume); ClientProtocol owns an independent event registry. PermissionManager remains the host authority. See session-events.md.

## Requirements and Routing

add_command(name, CommandRequirements, callback) is available directly and through protocol.add_command. Requirements accept a single Permission or a PermissionSet built with |. All required flags must be present. Permission::Any, a CommandRequirements default or PermissionSet{} requires an identified registered user even though it requires no flags. Permission::NoAuth explicitly creates Public requirements and is reserved for command access, not assignment. Join/resume use that value. Public access cannot combine participant flags.

Hosting decodes one JSON v4 frame and calls handleMessage, which validates requestId then calls handleCommand for every name. The handler builds a fresh context and invokes dispatch. Anonymous live peers execute only explicit Public commands. Identified actors, including public callers, are verified through permissions.hasPermissions; unknown identities reject. No identity or permission claim in payload replaces the connection's actor.

Cached contexts from closed/replaced peers, cached anonymous contexts after admission, and forged identity/connection pairs reject before callbacks. A trusted local call may supply a registered UserId without a connection; an entirely empty context rejects. Public callbacks handle optional identity; authenticated callbacks can dereference it. PermissionManager mutation methods separately enforce target/domain restrictions.

ResourceManager exposes typed internal functions. PermissionManager::registerCommands explicitly registers permissions.get, permissions.applyPreset and permissions.update through protocol.add_command. These adapters invoke permission features and the internal identity.updateUserPermissions coordinator; other registry features are not automatically network RPCs. See session-permission-commands.md for payloads/transactions.

On clients, on_event(name, callback) and OnEventSignature register incoming host notifications without permission requirements. Client event dispatch validates the currently admitted host connection generation, then invokes the void callback; no reply is generated. ClientConnection::processMessages routes correlated accepted/rejected replies to pending futures and messages without requestId to dispatch. sendEvent is host-only, exposed as protocol.sendEvent, and uses a bounded outbox. Client events are not server commands.

## Wire Results

For an explicitly registered example.echo requiring Permission1 and Permission3:

```json
{"version":4,"type":"example.echo","requestId":"command-1","payload":{"value":7}}
```

Success returns the callback result unchanged inside the wrapper:

```json
{"version":4,"type":"request.accepted","requestId":"command-1","payload":{"result":{"value":7}}}
```

Results may be objects, arrays, scalars or null. Error replies use request.rejected with exactly payload.code/message, preserving requestId when valid. The closed codes remain INVALID_REQUEST, INVALID_SESSION_CODE, INVALID_USER_TOKEN, NOT_ADMITTED, ALREADY_ADMITTED, ADMISSION_FAILED, UNKNOWN_COMMAND, FORBIDDEN and COMMAND_FAILED. Wire messages are sanitized Italian; raw callback failures are not exposed. Invalid/oversized output is converted to COMMAND_FAILED without retrying the callback. Ordinary admitted errors keep the connection open; pending errors, framing failures and stale peers close it.

## Asynchronous sendCommand

SessionClient::sendCommand(std::string, json) delegates and returns future<json>. Names are normalized and payloads must be objects. Invalid arguments, inactive client admission and queue limits reject synchronously; execution/transport/rejection errors complete the future exceptionally.

- ClientConnection queues at most 64 outstanding requests, allocates unique client-N IDs through ClientProtocol, handles partial I/O and correlates accepted/rejected outcomes, including out-of-order replies. Each request has five seconds from submission; timeout or connection loss fails remaining promises without replay. See session-client.md.
- The host uses the same client API over loopback. The desktop passes a single-use, memory-only token from the running server to the client for `session.resume`. Only a loopback peer with that token can claim the existing host identity. Stop invalidates any unused token.
- Same-manager callbacks cannot call sendCommand, avoiding host-actor escalation and worker deadlock. Nested direct dispatch with the original context and command registration remain supported.

## Synchronization and Stop

All network command callbacks run on Hosting; direct dispatch runs on its caller. One recursive execution mutex serializes them with registration, binding and close, preserving mutable callback state. Permission lookup is fresh for every request; changes affect subsequent executions, not a callback already begun. Independent provider calls retain their own synchronization rules.

Remote stop checks run before buffered peers, before/after select and at processInput entry. Immediately before handling a message the worker acquires the execution mutex and rechecks stop. Requests waiting for that mutex are discarded after stop; a request past the checkpoint may finish. Stop closes remaining buffers and peers rather than draining them. Final reply delivery is not guaranteed.

Lifecycle/load calls reject callback reentry before locks. Session destruction joins the hosting worker before destroying permission providers. Registry provider mutation is not concurrent with execution. No socket or permission mutex is held across arbitrary command callbacks.

## Verification

Sixteen play suites pass through the root CMake Debug build on Windows. Coverage includes requirement-mask pairs, explicit NoAuth/authenticated Any, trusted/current actors, local/TCP permission commands, transactional rollback, targeted live events, mixed replies, queue bounds and buffered-stop regressions; see development.md and session-events.md.
