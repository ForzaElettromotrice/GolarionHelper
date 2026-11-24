# Former play diagnostic tools (2026-11-23)

The host and client console programs were removed. These notes describe the former tools only; the desktop application uses the same session library directly.

## Console and Credential File

```powershell
./build/core/play/golarion_play_host.exe 127.0.0.1 42500 ./session-identities.json
./build/core/play/golarion_play_client.exe 127.0.0.1 42500 A7K2Q ./my-session-token.txt
```

Run separate terminals with the current printed code. Host syntax is IP/port/[archive]/[name]; the optional name follows an explicit archive path. Client syntax is IP/port/code/[token file], default client-token.txt. For another PC supply a reachable address and configure the host bind address appropriately. Linux names omit .exe. Both sides must be rebuilt for JSON v4.

A missing token file selects join; an existing valid file selects resume. The tool writes only the original 16 lowercase hex characters plus newline, never prints them, and reports effective flags as Italian Permesso1 through Permesso5/CambiaPermessi. No persistent preset label is inferred. Empty masks display nessuno. Invalid existing files fail before connecting and are preserved. Use one writer and one token file per participant/session. After discarding an archive, use a new token file.

New credential output uses an owned temporary file prepared before admission, owner-only permissions where supported, flush/close/rename after success and cleanup on failure. No application-level acknowledgement or power-loss durability guarantee is added. Ctrl+C/SIGTERM requests orderly disconnect; ordinary remote closure/replacement exits with failure. The console remains an admission/monitoring tool with no generic command prompt or frontend. See development.md for CMake builds.
