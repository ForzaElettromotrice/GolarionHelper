# Session Tokens and Identity Persistence

Status: token/identity persistence implemented 2026-11-10; JSON admission integration tested 2026-11-12. ProtocolManager owns the private registry and archive operations; Session only delegates. JSON join/resume uses these components as documented in `session-admission.md`. Client-side storage remains separate. There is no SessionId or display name.

## Credentials

- `UserToken` is a value type containing eight random bytes, encoded as exactly 16 lowercase hexadecimal characters. Parsing is strict and case sensitive; unlike SessionCode, tokens are not normalized.
- Generation uses OpenSSL Crypto `RAND_priv_bytes`; random-source failures propagate without a fallback. SHA-256 uses `EVP_Digest` over the raw eight bytes.
- The private `detail::UserTokens` registry maps stable UserIds to SHA-256 digests. It stores no original credential. Lookup hashes the supplied token and compares digests with `CRYPTO_memcmp`; malformed or unknown tokens return no identity.
- One credential is issued per registered user. Duplicate issuance is rejected without rotating the existing token. A generated digest collision is retried, with a limit of 32 attempts. Revocation/rotation is not implemented.
- The client needs to retain only the original token. UserId and current role are resolved by the host. A token belongs to the Session archive that issued it, rather than to an IP address, socket, or global account.

## Backend API

| Operation | Behavior |
| --- | --- |
| `Session::registerUser(UserId)` | Delegates to RoleManager; creates a Player with the existing validation rules. |
| `ProtocolManager::issueUserToken(const UserId &)` | Requires a known user and a previously saved/loaded archive. Stages a new credential and writes users, roles, and digests before installing it and returning the original token. Session exposes a delegation. |
| `ProtocolManager::resolveUserToken(std::string_view) const` | Returns the matching UserId or `nullopt`. Direct ProtocolManager method only; no Session getter or ResourceManager feature. |
| `ProtocolManager::saveIdentityState(const std::filesystem::path &)` | Writes the current identity snapshot and configures that archive for future issuance. Session exposes a delegation. |
| `ProtocolManager::loadIdentityState(const std::filesystem::path &)` | Restores validated identities, roles, and digests with hosting stopped. Session exposes a delegation. |

Create a new archive in an existing directory:

```cpp
golarion::play::Session session("host");
session.registerUser("alice");
session.saveIdentityState("identities.json");
const std::string token = session.issueUserToken("alice");
```

Reopen the same archive with the same immutable Host UserId:

```cpp
golarion::play::Session reopened("host");
reopened.loadIdentityState("identities.json");
```

The first save requires an unused path. An existing archive must be loaded before this ProtocolManager can overwrite it; subsequent saves to its configured path are allowed. Saving to another unused path switches the active archive after success. Parent directories are not created automatically. Use one writer per archive; cross-process locking is not implemented.

Issuance automatically saves the current users and roles together with the new digest. Later registration or role changes require an explicit `saveIdentityState`; there is no general autosave hook. Stopping/restarting hosting preserves the in-memory token registry and roles. Closing the application requires loading the archive into the next Session.

## SaveData and File Format

`IdentitySaveData` contains `users` (`UserRoleSaveData`) and `tokens` (`UserTokenSaveData`). RoleManager remains the sole runtime role authority and publishes `roles.toSaveData` and `roles.restoreSaveData` for ProtocolManager's trusted local use. These are not network commands.

The version-1 JSON archive has exactly these top-level fields:

```json
{
  "version": 1,
  "users": [{"userId": "host", "role": "host"}],
  "tokens": []
}
```

Each token record has `userId` and `hash`, with a 64-character lowercase hexadecimal SHA-256 digest. Participant role names remain the closed vocabulary `host`, `master`, `player`, and `spectator`. Any is only a command-access threshold: `"any"` is rejected in archives and `Role::Any` in role restoration. The archive schema/version is unchanged. Snapshots are sorted by UserId. The archive excludes original tokens, SessionId, SessionCode, socket/binding state, display names, sheets, and other future game state.

Loading validates the entire candidate before replacing live state: exact fields/types/version, strict JSON, known roles, valid unique IDs, exactly the existing immutable Host, unique token users/digests, and references to registered users. Duplicate JSON keys, a BOM, trailing data, nesting beyond 16, and files larger than 4 MiB are rejected. Failed validation preserves current roles, credentials, bindings, and archive selection.

Successful restoration clears old logical connections/bindings without resetting the connection-ID counter. It does not change the currently generated SessionCode. Host-wide code uniqueness and code persistence remain separately deferred.

## Writes and Synchronization

Private `identity_storage` writes to an exclusively created temporary file beside the destination, flushes/closes it, and atomically replaces the archive. Windows uses `CreateFileW`, `FlushFileBuffers`, and `MoveFileExW`; POSIX uses mode 0600, `fsync`, `rename`, and parent-directory synchronization. Temporary files are cleaned after failed writes. I/O and crypto errors propagate; a token is returned only after a successful save. The runtime candidate is installed only after that save succeeds.

ProtocolManager serializes tokens/files with its identity mutex and command execution/binding changes with its recursive execution mutex. Load additionally excludes lifecycle changes and is forbidden during hosting or from command callbacks. RoleManager synchronizes its data. Registry registration/removal remains outside the hosting lifetime; independent provider access retains its own threading contract. Join saves staged roles/credentials together; see `session-admission.md` and `session-commands.md`.

## Verification and Remaining Integration

`play_user_token_test` checks strict parsing, generation, owned values, and an independent SHA-256 vector. `play_user_tokens_test` checks deterministic collision/retry behavior, duplicate issuance, crypto failures, independent registries, and digest-only import/export. `play_identity_persistence_test` covers restoration, roles, corrupt archives, write failures/cleanup, existing-file protection, unknown token-user references, concurrent issuance with stable roles, hosting restrictions, and Session delegations. Separate writer/reader processes verify a real restart with only the original token retained by the simulated client.

All eleven play suites and the console-host restart smoke check pass on Windows with GCC C++20, strict warnings, nlohmann-json 3.12.0, and OpenSSL 3.0.15. CMake configuration and the POSIX implementation remain unexecuted locally.

JSON admission and authenticated registered-command dispatch are implemented; see `session-admission.md` and `session-commands.md`. Next are client-side ProtocolManager/token retention and concrete role/sheet commands. Local token issuance alone still does not bind a socket or register a command.
