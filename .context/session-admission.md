# JSON Session Admission

Status: implemented and tested, 2026-11-12. ProtocolManager accepts new users and resumes persisted identities over JSON v2/TCP. Admitted requests now use authenticated dispatch as documented in `session-commands.md`. Display names, client-side ProtocolManager, concrete role/sheet commands, and user-list events remain separate.

## Ownership and Entry Points

Session remains a container/facade; this step adds no Session methods. Configure its identity archive with `saveIdentityState` or `loadIdentityState`, then call `startHosting(ip, port)`. See `session-tokens.md` for archive rules.

Private `ProtocolManager::Hosting` reads bytes and calls the existing generic codec. It forwards complete messages to private `ProtocolManager::handleMessage(connectionId, message)`. Every request uses the same context/lookup/role-check/dispatch path. During construction ProtocolManager registers `session.join` and `session.resume` with `add_command` and minimum `Role::Any`, before publishing `protocol.add_command`. Pending connections have an optional empty UserId and satisfy only Any. Any is an access threshold, never a participant role.

The private implementation is split into `src/admission.hpp` and `src/admission.cpp`:

| Function | Responsibility |
| --- | --- |
| `handleMessage` / `handleCommand` | Validate request ID, construct a fresh context, dispatch by registered name, and build shared success/rejection envelopes and closure policy. No admission-name routing branch. |
| `registerAdmissionCommands` | Registers join/resume callbacks through `add_command` with minimum Any; ordinary duplicate checks prevent overriding either name. |
| `requirePendingAdmission` | Requires a pending network connection, checks the exact payload schema and session code, and returns its ConnectionId. |
| `admitNewUser` | Stages a new UserId, Player registration, token/hash, and connection binding; saves before committing runtime state. |
| `resumeUser` | Resolves an existing token, reads the current role through ResourceManager, and replaces the old binding with `bindUser`. |
| `makeAdmissionResult` | Creates a JSON callback result with UserId/current role; includes the original token only for join. Shared `makeCommandReply` wraps it as `request.accepted` / `payload.result`. |

ProtocolManager still publishes only `protocol.add_command`. Join/resume are entries in its command registry, not separate ResourceManager features. Registration and dispatch apply the same case-sensitive outer-whitespace normalization to every command name; `session.join` and ` session.join ` select the same built-in callback. See `session-commands.md`.

## Implemented Messages

All messages retain the existing four-byte big-endian length prefix and JSON v2 envelope. Request IDs must be nonblank strings of at most 128 UTF-8 bytes with no ASCII control characters. Valid IDs are echoed; invalid envelopes/IDs receive a rejection without an ID.

First admission:

```json
{"version":2,"type":"session.join","requestId":"join-1","payload":{"sessionCode":"A7K2Q"}}
```

Success is `request.accepted`, with exactly `userId`, `role` (`player`), and `userToken` inside `payload.result`. The public UserId is generated independently as `user-` plus a random hexadecimal nonce; it never exposes the issued credential. IDs are checked against current roles/bindings, with at most 32 candidate attempts, and final registration still rejects duplicates.

Resume:

```json
{"version":2,"type":"session.resume","requestId":"resume-1","payload":{"sessionCode":"A7K2Q","userToken":"0123456789abcdef"}}
```

Success is `request.accepted`, with exactly `userId` and the current `role` inside `payload.result`. It does not re-register the user, issue a new token, or reset their role. Both operations require the current SessionCode; lowercase letters are accepted. Tokens retain their strict lowercase hexadecimal format. Unknown fields, including client-chosen UserId, role, or displayName, are rejected. The former `session.joined` and `session.resumed` response types are retired; JSON envelope version remains 2.

Host-wide code uniqueness and code persistence remain deferred. Reconstructing a Session creates a new code; a returning client supplies that current code together with its original personal token.

## Registration, Persistence, and Synchronization

RoleManager now exposes `registerUserWithCommit(userId, commit)` through `roles.registerUserWithCommit`, using `RegisterUserWithCommitSignature = void(UserId, RegistrationCommit)` and `RegistrationCommit = std::function<void(const std::vector<UserRoleSaveData> &)>`. The existing `roles.registerUser` remains unchanged.

The new operation validates the ID, prepares a candidate role map with a Player, and invokes the callback with its sorted snapshot while holding the role mutex. Only a successful callback installs the candidate. An empty callback, invalid/duplicate ID, or callback exception preserves the previous roles. This is trusted local functionality; the callback must not reenter RoleManager or acquire ProtocolManager locks.

During join, ProtocolManager holds identity and connection locks, prepares the token registry and binding, and invokes the new role feature. Its callback writes the combined candidate roles and token hashes using the existing atomic archive writer. A failed registration/write restores the pending binding and discards staged credentials without removing or overwriting unrelated local role changes. Successful role/token/binding commits use already prepared state. The token is returned only in the private success reply; an interrupted connection after successful persistence can still lose that reply, and no recovery/acknowledgement flow is added here.

All RoleManager data operations use an internal mutex. Finish ResourceManager feature registration before hosting and stop the worker before teardown. ProtocolManager now serializes command registration and local/remote dispatch with a recursive execution mutex; independent provider access outside that path retains its own thread-safety contract.

Join locks execution, identity, logical connections, then roles. Archive load additionally holds lifecycle before execution and remains forbidden during hosting or from command callbacks. The storage callback writes supplied data without reentering a manager. Normal issuance/save uses identity then roles. Later local role changes still need an explicit save.

## Connections and Rejections

Hosting keeps a successful connection open. The five-second deadline applies only while its logical record has no UserId; a generic Any command can execute before admission but does not create an identity or reset that deadline. Admitted idle connections have no heartbeat timeout yet. Resume invalidates the previous logical binding immediately, and the worker closes that socket. A late disconnect cannot remove the replacement binding. Ordinary disconnect/stop removes presence while preserving identities, tokens, and roles.

The worker processes fragmented and coalesced frames, retains buffered following messages, and keeps at most one bounded response outstanding per peer. Input is capped at 65536 + 4 bytes. Socket I/O remains nonblocking, select waits at most 50 ms, and the existing limit is 32 total TCP peers, also constrained by platform select capacity.

Every rejection uses `request.rejected` with English `code` and Italian `message`:

| Code | Behavior |
| --- | --- |
| `INVALID_REQUEST` | Invalid frame/envelope/request ID closes after replying; invalid admission/callback payload closes a pending peer but keeps an admitted peer open. |
| `INVALID_SESSION_CODE` | Wrong/malformed session code; close the pending connection. |
| `INVALID_USER_TOKEN` | Malformed, unknown, or foreign token; close the pending connection without replacing an existing user. |
| `NOT_ADMITTED` | Unknown/closed/replaced connection or inconsistent/stale context; close after replying when possible. A valid pending connection is a usable anonymous context. |
| `ADMISSION_FAILED` | Archive not configured, storage/crypto failure, or provider failure; generic reply and close, keeping the worker alive. |
| `ALREADY_ADMITTED` | An admitted connection attempts another join/resume; preserve its identity and keep it open. |
| Command outcomes | Every name follows common lookup/permissions. Pending peers can execute Any; higher minima receive `FORBIDDEN`, unknown names receive `UNKNOWN_COMMAND`. Pending errors close the peer, while ordinary admitted errors keep it open. See `session-commands.md`. |

## Console and Verification

`golarion_play_host <IPv4 address> <port> [identity archive]` creates or loads the archive before listening. The default is `session-identities.json` in the working directory; use separate paths for separate Sessions. Existing archives must be valid and contain the same immutable Host UserId (`host` for this tool). Invalid archives are not replaced. The tool still prints the endpoint and SessionCode and waits for Ctrl+C. It has no client implementation.

`play_admission_test` covers TCP join/resume, strict payloads, replacement, persistence failures/retry, reconstruction, Session delegation, streaming/deadlines, and concurrent roles/admissions. Shared-dispatch coverage includes padded admission names, anonymous Any callbacks, forged payload actors, minimum-role rejection, common result wrappers, pending expiry after successful Any calls, and invalid outputs for both anonymous/admitted peers. It also executes buffered commands and checks callback failures; see `session-commands.md`. All eleven native suites and the console-host restart/resume smoke test pass on Windows GCC C++20 with strict warnings. CMake and POSIX execution remain unverified locally.
