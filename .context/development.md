# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The root build includes `core/sheet/` and `core/play/` through `core/CMakeLists.txt`. Play builds `golarion_play`, the Session-based console host, and eleven native C++ tests. Game/network, the console client, their model/process tests, and the play module's Python test dependency have been removed. Sheet targets and root build/test commands are unchanged.

## Session Hosting API

Session is the local backend entry point:

```cpp
golarion::play::Session session("host");
session.saveIdentityState("identities.json"); // New archive; load it instead when reopening.
session.startHosting("127.0.0.1", 42500);
// Application work continues while ProtocolManager processes network events.
session.stopHosting();
```

Port 0 requests an OS-assigned port, returned by startHosting. Startup errors propagate; stop/destruction join the worker before provider teardown. Configure an archive before admission; join otherwise receives `ADMISSION_FAILED`. The worker handles join/resume and registered commands without application polling. Local/remote execution and binding changes are serialized; lifecycle/load calls from callbacks are rejected to avoid self-join/lock cycles. Concrete role commands and the client remain separate. See `session-admission.md`, `session-commands.md`, and `session-tokens.md`.

## Standalone Play Backend

The module can be built without the sheet engine or frontend:

```sh
cmake -S core/play -B build/play-cmake
cmake --build build/play-cmake --config Debug
ctest --test-dir build/play-cmake -C Debug --output-on-failure
```

The host console accepts IP and port, plus an optional identity archive:

```powershell
./build/play-cmake/golarion_play_host.exe 127.0.0.1 42500
./build/play-cmake/golarion_play_host.exe 127.0.0.1 42500 ./my-session.json
```

Append `Debug/` to the executable directory for a multi-configuration CMake generator. The command is `golarion_play_host <IPv4 address> <port> [identity archive]`. The tool creates `Session("host")` and creates/loads the archive before listening; default `session-identities.json` is relative to the working directory. Use a separate archive per Session. It prints the actual listening port followed by `Codice sessione: XXXXX` and waits for Ctrl+C. The code stays unchanged during stop/restart of that live Session; reconstructing it generates another code because code persistence/uniqueness remain deferred. Its main thread does not process network events. Stop joins the worker and releases sockets/bindings, preserving the live roles/tokens and ID counter.

For LAN/VPN use, bind to the intended local IPv4 address or `0.0.0.0`; peers connect to a reachable address of that PC. There is no client executable yet: ProtocolManager client-side support is pending. Native tests use direct TCP peers for framing and admission. Tools do not configure firewall/router access.

## Manual Windows Build

CMake is absent from the current environment. GCC builds use the cached JSON header under `build/play/dependencies/include/`, OpenSSL Crypto, and standard thread/socket support. The existing local OpenSSL 3.0.15 installation is under `C:/Users/Daniele/miniconda3/Library`; adapt this path on another PC and ensure its `bin` directory is on PATH for the runtime DLL:

```powershell
New-Item -ItemType Directory -Path build/play -Force | Out-Null
$playCryptoRoot = "C:/Users/Daniele/miniconda3/Library"
$playSourceFiles = @(
    "core/play/src/admission.cpp",
    "core/play/src/resource_manager.cpp",
    "core/play/src/role_manager.cpp",
    "core/play/src/protocol.cpp",
    "core/play/src/hosting.cpp",
    "core/play/src/protocol_manager.cpp",
    "core/play/src/session.cpp",
    "core/play/src/session_code.cpp",
    "core/play/src/user_token.cpp",
    "core/play/src/user_tokens.cpp",
    "core/play/src/identity_storage.cpp"
)
$playCompilerOptions = @(
    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
    "-I", "core/play/include", "-I", "core/play/src", "-I", "build/play/dependencies/include",
    "-I", "$playCryptoRoot/include"
)
g++ @playCompilerOptions @playSourceFiles core/play/tools/host.cpp "$playCryptoRoot/lib/libcrypto.lib" -lws2_32 -o build/play/golarion_play_host.exe
if ($LASTEXITCODE -ne 0) { throw "Compilazione host fallita" }
foreach ($playTestName in @("session_code", "user_token", "user_tokens", "identity_persistence", "resource_manager", "role_manager", "protocol_manager", "session", "json_protocol", "hosting", "admission")) {
    g++ @playCompilerOptions @playSourceFiles "core/play/tests/${playTestName}_test.cpp" "$playCryptoRoot/lib/libcrypto.lib" -lws2_32 -o "build/play/play_${playTestName}_test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Compilazione fallita: $playTestName" }
    & "./build/play/play_${playTestName}_test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Test fallito: $playTestName" }
}
./build/play/golarion_play_host.exe 127.0.0.1 42500
```

Verification after common Any dispatch builds a fresh eleven-source library, host, and all eleven native suites under `build/play/any-commands/`; the tested host is refreshed under `build/play/`. Existing tests cover anonymous Any commands, all 25 threshold comparisons, rejection of Any assignment/restoration, protected built-in names/padded variants, optional/stale contexts, common join/resume results, pending deadlines, serialized callbacks/binding, nested dispatch, current roles, typed failures, invalid output, and lifecycle reentry. All role, token, archive, codec, connection, and hosting suites pass. The real console-process restart/resume smoke test checks padded admission names and `request.accepted` / `payload.result`, default/explicit archives, and `UNKNOWN_COMMAND` for unregistered requests. The tool has no concrete command provider yet. Socket tests require permissions because the sandbox blocks loopback with WSA error 10013. CMake and POSIX remain unverified locally.

The later buffered-stop correction is verified in `build/play/stop-buffered/`: rebuild `hosting.cpp`, replace that object in a copy of the verified library, and rebuild the affected admission, hosting, ProtocolManager, identity-persistence executables and console host with the same strict compiler options. All four suites and the console restart/resume smoke check pass. Two new cases in the admission suite reproduce buffered work after stop and work waiting for the execution mutex; both fail against the previous library and pass against the corrected one. The tested host is refreshed under `build/play/`. No CMake target/dependency or public API changes are needed; the admission suite retains its existing 40-second CTest timeout.

Historical Windows/Linux/Tailscale results and the old Linux send-timeout issue are archived in `history/2026-11-10-game-network-prototype.md`. They describe removed code, not current launch instructions; verify them again when implementing the new client.

## Dependencies

The sheet and play modules require nlohmann-json 3.12 or newer. CMake resolves `nlohmann_json::nlohmann_json`; play publicly exposes JSON in its callback API. Play also requires OpenSSL 3.0+ Crypto (`OpenSSL::Crypto`) for random tokens and SHA-256, linked privately alongside `Threads::Threads` and, on Windows, `ws2_32`. The backend and all play tests are native C++; Python is not required by their CMake targets. Local verification used the existing OpenSSL 3.0.15 headers, import library, and runtime DLL without installing packages.

Manual builds use the official nlohmann/json 3.12.0 single header cached at `build/play/dependencies/include/nlohmann/json.hpp` (ignored), previously downloaded from `https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp`. Replace that include path with your installed package when appropriate.
