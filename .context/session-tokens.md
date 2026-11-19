# Session Tokens and Identity Persistence

ProtocolManager owns the credential registry and archive. Archive v3 adds a display name to each stored identity alongside its permission mask. Session delegates identity operations to ProtocolManager and PermissionManager.

## Credentials and API

UserToken holds eight random bytes, encoded as exactly 16 lowercase hexadecimal characters. Parsing is strict and case sensitive. OpenSSL RAND_priv_bytes generates credentials; SHA-256 via EVP_Digest hashes the raw bytes. Private UserTokens stores only digests and uses CRYPTO_memcmp for comparisons. One token is issued per user; duplicate issuance rejects without rotation. Digest collisions have bounded retries. Revocation/rotation is not implemented.

| Operation | Contract |
| --- | --- |
| Session::registerUser(UserId) | Delegates to PermissionManager; registers Player flags without ChangePermissions. |
| issueUserToken(UserId) | Requires a registered user and configured archive; persists the candidate credential before returning its original token. Session delegates. |
| resolveUserToken(string_view) | ProtocolManager-only lookup returning optional UserId; malformed/unknown credentials return nullopt. |
| saveIdentityState(path) | Saves detached users/permission masks and digests, configuring this archive for later issuance/admission. Session delegates. |
| loadIdentityState(path) | Requires hosting stopped and no callback reentry; validates all data before replacing users/digests and clearing bindings. Session delegates. |

The first save needs an unused path in an existing directory. Load an existing archive before replacing it; later saves to the configured path are allowed. Use one writer per archive. Issuance/admission and the two permission-mutation commands automatically persist their transactions. Trusted direct registration/setters remain in-memory until explicit saveIdentityState or a coordinated commit. The command coordinator snapshots token digests before the permission lock; its storage callback never reenters managers. Hosting stop preserves memory; application restart requires loading the archive.

## Archive Format

IdentitySaveData contains hostId, vector<UserPermissionsSaveData> and vector<UserTokenSaveData>. PermissionManager owns users/flags and provides trusted permissions.toSaveData/restoreSaveData/registerUserWithCommit features. They are internal registry functions, not network commands.

```json
{
  "version": 3,
  "hostId": "host",
  "users": [
    {"userId": "host", "permissions": 63, "displayName": "Marta"},
    {"userId": "alice", "permissions": 18, "displayName": "Alice"}
  ],
  "tokens": []
}
```

Top-level fields are exact. User entries have userId, permissions and displayName. Token entries store userId and a 64-character lowercase SHA-256 verifier. The reader accepts v2 with unset names and preserves each UserId, mask and hash. The next successful write upgrades to v3. A missing legacy name remains unset until supplied and appears as Host or Giocatore in presence. Masks must be nonnegative integers with known bits. The hostId must match the Session constructor and its mask must remain 63.

The reader rejects unsupported versions, old role fields, duplicate keys/users/tokens/verifiers, unknown token users, invalid IDs/hashes/masks, BOM, trailing data, oversized files and excessive nesting. Maximum archive size is 4 MiB and nesting is bounded at 16. The user requested discarding legacy archives rather than conversion: version 1 is rejected without deletion or silent overwrite. The one old local identity archive found before integration was removed at that explicit request. Do not add automatic startup deletion.

Load constructs/validates the candidate token registry, checks explicit Host identity and restores the validated permission snapshot before swapping credential state or clearing bindings. Failures preserve current users, credentials, connections and configured archive. Writes use an exclusive temporary file, flush and atomic replacement with platform-specific filesystem handling; errors do not commit candidate credentials or return an unusable original token.

## Client Retention and Verification

Clients store only the original token in a dedicated per-participant/session file; the server resolves identity and effective permissions. IP/socket changes do not change identity. The console selects join for a missing file and resume for an existing valid one. After discarding an archive, use a fresh client token file. See session-client.md.

The identity suite covers corruption, unknown masks/versions/host mismatch, atomic write failure, arbitrary saved masks and restart. The permission command suite verifies automatic saving, unchanged memory/original archive after write failure, concurrent updates and original-token resume after full host reconstruction. All seventeen play suites pass through the root CMake build on Windows, including live events after committed changes; see development.md and session-events.md.
