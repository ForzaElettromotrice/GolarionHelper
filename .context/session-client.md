# Client Sessions and Admission

Status: implemented 2026-11-16. The native client connects, joins/resumes, monitors disconnection and exposes synchronized state. Generic outgoing commands, host events/user snapshots, automatic reconnect and role updates after admission are separate steps.

## Session Modes and Ownership

- `Session::createHost(UserId)` constructs an immutable `SessionMode::Host`; the existing explicit `Session(UserId)` remains equivalent for compatibility.
- `Session::createClient()` constructs `SessionMode::Client`. Named factories return values with guaranteed copy elision; Session and its managers remain noncopyable/nonmovable.
- Every Session owns ResourceManager then ProtocolManager. Only host Sessions construct the optional RoleManager, generate a local SessionCode, allocate the server token registry and publish `protocol.add_command`/`roles.*`. A client has no invented local host or authoritative role data.
- Session methods delegate. Only its host-only `registerUser` delegation checks that RoleManager exists. Networking, mode enforcement, threads and client state belong to ProtocolManager.
- SessionMode is independent from participant Role. An admitted Master is still a client. Disconnect never changes mode. Host-only methods on clients and client-only methods on hosts throw `std::logic_error`; the existing noexcept `verifySessionCode` returns false in client mode.
- Both destructors stop/join their relevant worker before subsystem teardown. A client ProtocolManager neither publishes nor removes another provider's ResourceManager features.

## API

The following methods exist on Session and ProtocolManager. Low-level ProtocolManager construction accepts `SessionMode`, defaulting to Host for existing callers.

| Method | Contract |
| --- | --- |
| `std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code)` | Starts an owned worker, connects to numeric IPv4/nonzero port and submits `session.join`. Completes after a correlated, validated admission result. |
| `std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token)` | Same transport flow with `session.resume`; supplies the existing credential and preserves server identity/current role. A rejection is an error, with no new-user fallback. |
| `void disconnectFromHost()` | Requests cancellation, joins the worker and releases the socket. Idempotent for an inactive client. |
| `ClientSnapshot getClientState() const` | Returns a detached, synchronized state/optional UserId/optional Role snapshot. Contains no credential. |

`ClientAdmission` contains UserId, Role and optional UserToken. Only join returns a newly issued token; resume returns no token. The caller retains its original credential independently of connection lifetime. `ClientSnapshot` clears identity/role on disconnect. Its role is the last admission result, not a live synchronized user list.

`ClientConnectionState` is closed: Disconnected, Connecting, Admitting, Connected. Connected means admission succeeded. IP and typed code/token arguments are owned by value across asynchronous work. Invalid mode/address/port and overlapping active admissions throw synchronously; network/reply failures are delivered through the future. A failed/disconnected client can start another attempt after the previous worker is joined.

## Transport and Replies

- Private nested `ProtocolManager::ClientConnection`, implemented in `src/client_connection.hpp/.cpp`, owns the socket, buffers, promise, snapshot mutex, stop flag and worker. ProtocolManager serializes client lifecycle with its client mutex; the worker never acquires that mutex. Snapshot reads use the connection's state mutex.
- Both host and client reuse private `socket_support.hpp`: RAII socket/runtime ownership, numeric IPv4 validation, nonblocking configuration, platform errors and SIGPIPE suppression. The host's buffered-stop checkpoint remains intact.
- Nonblocking connect recognizes pending-connect errors and checks `SO_ERROR` after select readiness. Connect has a five-second deadline. Sending and receiving admission share a separate five-second deadline. Select waits at most 50 ms between cancellation checks; no application polling is needed to progress networking.
- Client requests use the existing JSON v2 codec/4-byte prefix/64 KiB limit. IDs `client-N` increase over attempts within one manager. Reply requestId must match exactly; no absent/mismatched/legacy response is accepted.
- Success must be `request.accepted` with exactly `payload.result`: join requires UserId, participant role and token; resume requires UserId/role only. Strict enum parsing rejects Any/unknown roles and unknown error names. User IDs and credentials are validated, and duplicate keys/UTF-8/framing checks reuse the codec.
- Known `request.rejected` codes become existing `ProtocolError` values. Malformed responses become InvalidRequest. Transport/cancellation/timeout failures use `std::system_error`; pending futures are completed during cleanup rather than left broken or waiting.
- A complete admission reply is processed before any following bytes, independently of TCP grouping. Remaining input stays in ClientConnection's buffer for the next stage. In the current admission-only slice unsupported following data closes the connection, while the already completed admission result and original token remain available to the caller. The previous implementation incorrectly rejected the first reply when extra bytes arrived in the same read; the regression reproduces that failure against the initial library and passes with the correction.
- After admission the worker monitors EOF/reset. This slice supports no further host events or outstanding commands; unsolicited data closes the client. No heartbeat timeout or automatic reconnection is introduced. A future already completed successfully remains a valid admission result if the socket subsequently closes.

## Console and Credential File

```powershell
./build/play/golarion_play_host.exe 127.0.0.1 42500 ./session-identities.json
./build/play/golarion_play_client.exe 127.0.0.1 42500 A7K2Q ./my-session-token.txt
```

Run in separate terminals and replace the sample code with the host's printed code. For another PC, supply a reachable host address and bind the host accordingly. The client syntax is `golarion_play_client <IPv4 address> <port> <session code> [token file]`; default is `client-token.txt` in the working directory. See `development.md` for builds and Linux executable naming.

The tool reuses `tools/console.hpp` for port parsing and stop handlers. A missing credential file selects join; an existing valid one selects resume. It stores only the original 16 lowercase hex characters plus newline, never prints the credential, and preserves existing files on rejection. Malformed/oversized files fail before connecting. A new-file temporary stream is prepared before admission, restricted to owner read/write where supported, then flushed/closed and renamed after successful join; failures clean the temporary path. No application-level acknowledgement or power-loss durability guarantee is added.

Use one writer and one dedicated token file per participant/session; there is no global registry keyed by IP or the nonpersistent SessionCode. The default generated token file/temporary names are ignored by Git. The library returns credentials but does not choose their storage path. An ordinary host disconnect/replacement exits the console with a failure status; Ctrl+C/SIGTERM requests orderly disconnect. User-facing messages are Italian and avoid internal exception details.

## Verification (2026-11-16)

- `play_session_test` adds factory/mode isolation and delegated misuse checks.
- `play_client_test` now has fourteen named scenarios. Existing mode/admission/replacement/retry/error coverage is extended with twenty malformed-wire cases plus a zero-length prefix, partial-prefix/body EOF, a valid exact 65536-byte JSON envelope, slow progress that must not reset the five-second deadline, and complete/fragmented following messages grouped with admission. Protocol and transport errors must leave no pending future or stale active identity.
- A concurrent native case keeps eight clients alive and reconstructs the entire host three times from its archive, changing saved Master/Spectator roles and using each new current code. All 24 resumes preserve the original eight identities/tokens, expose the restored role and issue no new token. Another case races two starts, then performs twenty rounds of immediate cancellation with two simultaneous disconnect calls and verifies that the next connection succeeds.
- All twelve native suites pass with Windows GCC C++20 and strict warnings. The expanded client suite also passes while the other regressions run concurrently: about 16.6 seconds, below its existing 40-second CTest limit. Outputs are in ignored `build/play/client-validation/`.
- `extended_process_test.py` in that directory checks eight separate client console processes through three host-process restarts, abrupt client exits, connection replacement, restored roles, filenames containing spaces, LF/CRLF/no-newline token files, seven corrupt credential forms, credential preservation on save failure, and retention of a valid token when an unsolicited frame follows admission in the same send. The earlier two-participant smoke check remains under `build/play/client-admission/`.
- `coalesced_before_test.exe` records the reproduced initial failure. The permanent native test and real-process case cover the correction. Source tests remain in `core/play/tests/client_test.cpp`; the Python/probe artifacts are optional local diagnostics and add no CMake dependency.
- CMake still lists twelve library sources, twelve native suites and two console tools. Windows CMake/CTest are unavailable on PATH. Ubuntu 24.04 is present through WSL, but its compiler/CMake/OpenSSL development headers were unavailable when checked, so CMake/POSIX execution and physical two-PC networking remain unverified. No packages or system configuration were changed.
