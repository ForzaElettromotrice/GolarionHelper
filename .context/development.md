# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The root build includes core/sheet and core/play through core/CMakeLists.txt. Play builds golarion_play, both Session-based console tools and twelve native C++ suites. Legacy Game/network and their old model/process tests remain removed. Python is used only for an optional local console-process check, not by the CMake targets. Sheet targets and root build/test commands are unchanged.

## Session Hosting API

Session is the local backend entry point:

```cpp
auto session = golarion::play::Session::createHost("host");
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

Append `Debug/` to the executable directory for a multi-configuration CMake generator. The command is `golarion_play_host <IPv4 address> <port> [identity archive]`. The tool creates `Session::createHost("host")` and creates/loads the archive before listening; default `session-identities.json` is relative to the working directory. Use a separate archive per Session. It prints the actual listening port followed by `Codice sessione: XXXXX` and waits for Ctrl+C. The code stays unchanged during stop/restart of that live Session; reconstructing it generates another code because code persistence/uniqueness remain deferred. Its main thread does not process network events. Stop joins the worker and releases sockets/bindings, preserving the live roles/tokens and ID counter.

For LAN/VPN use, bind to the intended local IPv4 address or `0.0.0.0`; clients connect to a reachable host address. The client requires a nonzero port and the code printed by the host:

```powershell
./build/play-cmake/golarion_play_client.exe 127.0.0.1 42500 A7K2Q ./my-session-token.txt
```

Replace the example code with the current one. Use separate terminals for host/client. Linux uses the same arguments with executable names without `.exe`. The optional token file defaults to `client-token.txt` in the working directory. A missing file selects join and saves the original token; an existing valid file selects resume. Use one token file per participant/session and one writer per file. Invalid files/tokens are preserved and reported as errors. Ctrl+C disconnects; no command/user-list frontend exists yet. See `session-client.md`. Tools do not configure firewall/router access.

## Manual Windows Build

CMake/CTest are not available on PATH in the current environment. GCC builds use the cached JSON header under `build/play/dependencies/include/`, OpenSSL Crypto, and standard thread/socket support. The existing local OpenSSL 3.0.15 installation is under `C:/Users/Daniele/miniconda3/Library`; adapt this path on another PC and ensure its `bin` directory is on PATH for the runtime DLL:

```powershell
New-Item -ItemType Directory -Path build/play -Force | Out-Null
$playCryptoRoot = "C:/Users/Daniele/miniconda3/Library"
$playSourceFiles = @(
    "core/play/src/admission.cpp",
    "core/play/src/client_connection.cpp",
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
g++ @playCompilerOptions @playSourceFiles core/play/tools/client.cpp "$playCryptoRoot/lib/libcrypto.lib" -lws2_32 -o build/play/golarion_play_client.exe
if ($LASTEXITCODE -ne 0) { throw "Compilazione client fallita" }
foreach ($playTestName in @("client", "session_code", "user_token", "user_tokens", "identity_persistence", "resource_manager", "role_manager", "protocol_manager", "session", "json_protocol", "hosting", "admission")) {
    g++ @playCompilerOptions @playSourceFiles "core/play/tests/${playTestName}_test.cpp" "$playCryptoRoot/lib/libcrypto.lib" -lws2_32 -o "build/play/play_${playTestName}_test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Compilazione fallita: $playTestName" }
    & "./build/play/play_${playTestName}_test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Test fallito: $playTestName" }
}
./build/play/golarion_play_host.exe 127.0.0.1 42500
```

Verification on 2026-11-16 builds all twelve sources into `build/play/client-admission/libgolarion_play.a`, then both console tools and twelve native suites with GCC C++20 and warnings as errors. All suites pass, including existing host buffered-stop regressions and new client mode/admission/streaming/cancellation/timeout/network-error cases. A real-process check in `build/play/client-admission/client_process_smoke.py` verifies two participants, default/explicit token files, client and host restarts, replacement, rejected credentials and temporary-file cleanup. The verified host and client binaries are refreshed under `build/play/`. No new dependency is required.

Extended validation and the TCP-grouping correction use `build/play/client-validation/`: rebuild the private client-connection object into a copy of the verified library and relink both tools/all twelve suites. All suites pass, including a parallel run with the expanded client suite completing in about 16.6 seconds against its existing 40-second limit. The permanent client tests add eight simultaneous participants through three host reconstructions, twenty cancellation/disconnect races, twenty malformed-wire forms, partial EOF, an exact maximum-size frame, and a slow-input deadline case. `extended_process_test.py` checks eight console clients and three host-process restarts plus credential-file failure cases. `coalesced_before_test.exe` reproduces the original grouping defect; the corrected native/process tests preserve the complete admission/token before handling subsequent unsupported data. Verified binaries are refreshed under build/play.

Loopback execution needs local socket permissions in this environment because the sandbox otherwise denies connects with WSA error 10013. CMake configuration and POSIX execution remain unverified: Windows CMake/CTest are unavailable on PATH; WSL Ubuntu 24.04 exists but lacked an available compiler, CMake and OpenSSL development headers when inspected. No environment packages were installed. The Windows refused-connection test waits within the actual five-second connect deadline and verifies the socket error rather than mistaking normal asynchronous refusal latency for a client failure.

Historical Windows/Linux/Tailscale results and the old Linux send-timeout issue are archived in `history/2026-11-10-game-network-prototype.md`. They describe removed code, not current launch instructions; verify them again when implementing the new client.

## Dependencies

The sheet and play modules require nlohmann-json 3.12 or newer. CMake resolves `nlohmann_json::nlohmann_json`; play publicly exposes JSON in its callback API. Play also requires OpenSSL 3.0+ Crypto (`OpenSSL::Crypto`) for random tokens and SHA-256, linked privately alongside `Threads::Threads` and, on Windows, `ws2_32`. The backend and all play tests are native C++; Python is not required by their CMake targets. Local verification used the existing OpenSSL 3.0.15 headers, import library, and runtime DLL without installing packages.

Manual builds use the official nlohmann/json 3.12.0 single header cached at `build/play/dependencies/include/nlohmann/json.hpp` (ignored), previously downloaded from `https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp`. Replace that include path with your installed package when appropriate.
