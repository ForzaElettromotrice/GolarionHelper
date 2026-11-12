# JSON Command Dispatch

Status: unified dispatch implemented and tested, 2026-11-12. Every request uses the command registry, including built-in join/resume with minimum Any. Other subsystems register commands through `protocol.add_command`. Session remains a facade; concrete role/sheet commands, user-list events, and client-side ProtocolManager are separate steps.

## Existing Structure and Routing

The generic JSON v2 codec is unchanged. Hosting decodes one message and calls ProtocolManager's `handleMessage`, which validates requestId then calls `handleCommand` for every name. ProtocolManager registers `session.join` and `session.resume` via `add_command` before exposing that feature to other subsystems. They use minimum `Role::Any`; there is no separate admission-name router. Duplicate checks and common outer-whitespace normalization protect both built-in names.

`handleCommand` builds a fresh context with `makeContext(connectionId)` and calls `dispatch(message.type, context, message.payload)`. Dispatch looks up the registered name, queries current minimum-role permission through `roles.hasAtLeastRole` with `const std::optional<UserId> &`, then invokes the original stored callback. Live pending connections have no UserId and satisfy only Any; Any is not assignable to a participant. Supplied unknown identities are rejected rather than treated as anonymous. Client-supplied UserId/role fields cannot override the actor. Command callbacks retain their mutable state and may register commands or perform nested dispatch.

When a context contains a ConnectionId, dispatch also verifies record existence and equality of its optional UserId. Cached contexts from closed/replaced connections, cached anonymous contexts after admission, and forged identity/connection combinations are rejected before callbacks. Local trusted calls may use a known UserId without a ConnectionId; a context with neither is rejected. Authentication-required callbacks can dereference `context.userId`; Any callbacks must handle its absence.

Wire helpers stay in the existing `src/admission.cpp`; no second dispatcher or codec is introduced. `handleCommand`, `makeCommandReply`, and shared `makeRejection` are private ProtocolManager methods. Only `protocol.add_command` is published through ResourceManager.

## Requests and Results

The envelope's `type` is the registered command name; `payload` must be an object. For example, after a provider registers `example.echo`:

```json
{"version":2,"type":"example.echo","requestId":"command-1","payload":{"value":7}}
```

Successful execution returns `request.accepted`, echoing the request ID:

```json
{"version":2,"type":"request.accepted","requestId":"command-1","payload":{"result":7}}
```

`result` may be an object, array, scalar, or null. Join returns `{userId, role, userToken}` there; resume returns `{userId, role}`. Both use this same success envelope. The wrapper keeps the codec's object-payload contract. The codec still enforces UTF-8, JSON-compatible values, nesting, and the 64 KiB envelope limit. Results that cannot be encoded produce `COMMAND_FAILED`; the callback is not retried and any effects it already performed are not automatically rolled back. Input framing errors remain `INVALID_REQUEST` and close the connection.

## Typed Errors

Public `protocol_error.hpp` defines the closed `ProtocolErrorCode` enum and `ProtocolError`, which derives from `std::invalid_argument` and exposes `code()`. Existing C++ catches for manager validation failures remain compatible. Dispatch uses typed failures for invalid payload/context, stale identity, unknown command, insufficient role, and unavailable/failing role checks. Callback exceptions still propagate unchanged through direct C++ dispatch.

The wire handler maps failures without parsing exception text:

| Failure | Wire code |
| --- | --- |
| Unknown registered name | `UNKNOWN_COMMAND` |
| Insufficient current minimum role, or callback's typed permission failure | `FORBIDDEN` |
| Callback `std::invalid_argument` or JSON access/type exception | `INVALID_REQUEST` |
| Failed role service, other callback exception (including non-standard throws), or unencodable result | `COMMAND_FAILED` |
| Unknown/closed/replaced connection, stale or inconsistent context | `NOT_ADMITTED` |

`makeRejection` is shared with admission/framing and emits English codes with generic Italian messages. Internal exception details are not sent. Ordinary command failures keep an admitted connection open; errors on pending connections and stale contexts close the peer. Successful generic Any commands keep it pending without changing identity or the five-second deadline. `COMMANDS_UNAVAILABLE` is retired. A provider can raise `ProtocolError` for a documented outcome; it remains responsible for target-specific domain permissions and payload validation.

## Execution and Lifecycle

One recursive execution mutex serializes command registration, local/remote dispatch, admission, binding/replacement, and closing. Nested dispatch/registration and connection operations inside callbacks remain supported. No connection or role mutex is held while invoking a command callback. A replacement/close waits for an already executing callback; after it completes, stale contexts cannot start another command.

Remote callbacks run on the hosting worker, while direct calls execute on their caller's thread under the same execution mutex. Slow callbacks delay other commands and stop waits for them; no asynchronous command executor or cancellation is added. Providers called independently outside dispatch retain their own threading contracts. ResourceManager feature registration/removal remains restricted to outside the hosting lifetime.

For network input, the final execution checkpoint is inside Hosting's `processInput`: acquire the existing execution mutex, check `stopRequested`, then call `handleMessage`. A request that waited for this mutex must pass the stop check afterward; decoding or buffering alone does not grant execution. A request past the checkpoint is already in progress and may finish, including its nested operations. Stop discards remaining input/output buffers and joins the worker rather than draining requests. The completed request's reply may not be delivered before its socket closes. Direct trusted local dispatch remains available independently of hosting.

Lifecycle operations (`startHosting`, `stopHosting`, `loadIdentityState`) reject calls made from the same manager's command callbacks with `std::logic_error`, before acquiring lifecycle locks. A thread-local RAII dispatch stack also covers nested callbacks. This prevents self-join and lock cycles with a simultaneous external stop. Request lifecycle changes from application code after callback completion.

Lock order during admission is execution, identity, connections, then roles. Archive load uses lifecycle, execution, identity, connections, then roles. Stop never holds the execution lock while joining the worker; it acquires it only for final binding cleanup. Normal archive save/token issuance retains identity-then-role ordering.

## Verification

The existing suites are extended rather than adding another transport fixture. ProtocolManager tests cover anonymous Any/local authenticated dispatch, duplicate built-in names including padded variants, missing role providers, unknown identities, forged/stale contexts, serialized mutable callbacks/registration, binding/close waiting for an executing callback, and lifecycle reentry. Admission TCP tests exercise anonymous Any commands, pending expiry, padded join/resume through common dispatch, buffered/large commands, authenticated actors, live promotions/demotions, JSON result shapes, nested registration/dispatch, simultaneous local/remote calls, replacement, sanitized failures, and invalid/oversized results. Invalid output is not retried; admitted peers stay open while pending peers close.

The full eleven-suite run passed after unified Any dispatch. The subsequent stop correction passes the four affected suites (admission, hosting, ProtocolManager, identity persistence) and real console-host restart/resume smoke test on Windows GCC C++20 with strict warnings. The two new admission regressions fail against the previous library and pass against the correction. The console has no concrete game-command provider, so unknown requests receive `UNKNOWN_COMMAND`. CMake and POSIX execution remain unverified locally. Latest outputs are under `build/play/stop-buffered/`; see `development.md`.
