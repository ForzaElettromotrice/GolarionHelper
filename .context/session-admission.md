# Session Admission

Status: host/client numeric-permission admission integrated 2026-11-17. Session hosts own PermissionManager. ProtocolManager registers session.join and session.resume with add_command and Permission::Any = 0, using the common JSON v3 dispatch path. Any is command-only public access; anonymous participants are not given an assignable permission.

## Requests and Results

Join requests have exactly sessionCode:

```json
{"version":3,"type":"session.join","requestId":"client-1","payload":{"sessionCode":"A7K2Q"}}
```

The host generates an independent UserId and credential, registers the Player preset (mask 7), persists the identity and keeps the admitted connection open:

```json
{"version":3,"type":"request.accepted","requestId":"client-1","payload":{"result":{"userId":"user-example","permissions":7,"userToken":"0123456789abcdef"}}}
```

Resume requests have exactly sessionCode/userToken. Success returns the same UserId and current permissions without issuing a token:

```json
{"version":3,"type":"session.resume","requestId":"client-2","payload":{"sessionCode":"A7K2Q","userToken":"0123456789abcdef"}}
{"version":3,"type":"request.accepted","requestId":"client-2","payload":{"result":{"userId":"user-example","permissions":18}}}
```

Permission masks are nonnegative JSON integers containing only the six known flag bits; zero is valid for an identified user. Role strings/fields, floats, booleans, negative values and unknown bits are not accepted as permissions. Preset names are not persisted or returned. A non-host may have mask 63 without becoming the immutable Host.

Both callbacks require a live pending connection: local calls without a connection fail, and an already admitted caller gets ALREADY_ADMITTED. The payload schema rejects extra fields such as caller-selected identity, permissions or displayName. SessionCode normalizes lowercase; tokens are strict 16-character lowercase hexadecimal. JSON v2 peers are rejected before admission/credential issuance. Host-wide code uniqueness/persistence and display names remain deferred.

## Transaction and Lifecycle

Join holds execution, identity and connection locks, stages a new digest/binding and calls permissions.registerUserWithCommit. Its callback writes version-2 IdentitySaveData with the explicit Host ID, candidate users/permission masks and digests. The host ID is obtained before entering the permission callback. Only successful storage installs the staged user/token registry; failures roll back the logical binding and return ADMISSION_FAILED. Public UserIds are generated independently from credentials, with bounded collision retries.

Resume resolves the existing digest and obtains permissions.getPermissions, then replaces any previous binding for the UserId. Stale messages/late disconnects cannot remove the new association; Hosting closes the replaced socket. It does not issue a new token or reset custom/empty masks. Cross-application restoration requires the configured archive and current SessionCode.

Pending peers have five seconds from acceptance; successful generic Any commands do not admit them or renew this deadline. Admitted idle peers have no heartbeat timeout. Limits remain 32 TCP peers, bounded input/output and nonblocking I/O. Disconnect and hosting stop remove presence while preserving users, permissions and credentials.

Malformed framing and pending command/admission errors close the socket. Ordinary command errors after admission keep it open. The nine typed rejection codes and sanitized Italian messages are in session-commands.md. Built-in admission names are protected from duplicate registration, including padded variants.

## Verification

Admission/hosting/client tests cover strict schemas, staged failures, masks, replacement, concurrency and buffered stop. All sixteen play suites pass through the root CMake build on Windows, including permission mutation autosave, token-preserving restart and targeted live notifications. Admission completes before following event frames are processed; see session-events.md and development.md.
