# Historical Standalone Play Build — 2026-11-17

Superseded by the user request to configure the repository root with -S . -B build. The following records the earlier standalone verification only. The ignored build directory and its temporary tools were removed before the workflow change; verify tool/dependency availability before reusing historical paths.

# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The root build includes core/sheet and core/play through core/CMakeLists.txt. Play builds golarion_play, both Session-based console tools and fourteen native C++ suites, including integrated PermissionSet/PermissionManager. Legacy Game/network and their old model/process tests remain removed. Python is used only for an optional local console-process check, not by the CMake targets. Sheet targets and root build/test commands are unchanged.

## Session Hosting API

Session is the local backend entry point:

```cpp
auto session = golarion::play::Session::createHost("host");
session.saveIdentityState("identities.json"); // New archive; load it instead when reopening.
session.startHosting("127.0.0.1", 42500);
// Application work continues while ProtocolManager processes network events.
session.stopHosting();
```

Port 0 requests an OS-assigned port, returned by startHosting. Startup errors propagate; stop/destruction join the worker before provider teardown. Configure an archive before admission; join otherwise receives `ADMISSION_FAILED`. The worker handles join/resume and registered commands without application polling. Local/remote execution and binding changes are serialized; lifecycle/load calls from callbacks are rejected to avoid self-join/lock cycles. Concrete role commands remain separate; client admission and host/client sendCommand are implemented. See `session-admission.md`, `session-commands.md`, and `session-tokens.md`.

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

## Verified Windows CMake Build (2026-11-17)

CMake 3.31.10 was prepared inside ignored `build/play/dependencies/cmake/`, without changing system configuration. The official nlohmann/json 3.12.0 source was configured and installed with its own CMake files under `build/play/dependencies/json-install/`. The existing GCC 10.3, Ninja and OpenSSL 3.0.15 installation are reused. CMake finds the real JSON and OpenSSL package targets; no manual compiler or linker command is required.

The verified standalone commands from the repository root are:

```powershell
$playCMake = "./build/play/dependencies/cmake/cmake/data/bin/cmake.exe"
$playCTest = "./build/play/dependencies/cmake/cmake/data/bin/ctest.exe"
$playJsonPrefix = (Resolve-Path "build/play/dependencies/json-install").Path
& $playCMake -S core/play -B build/play-cmake -G Ninja -DCMAKE_CXX_COMPILER=C:/TDM-GCC-64/bin/g++.exe -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$playJsonPrefix" -DOPENSSL_ROOT_DIR=C:/Users/Daniele/miniconda3/Library -DOPENSSL_CRYPTO_LIBRARY=C:/Users/Daniele/miniconda3/Library/lib/libcrypto.lib -DBUILD_TESTING=ON
& $playCMake --build build/play-cmake --parallel 4
$env:PATH = "C:/Users/Daniele/miniconda3/Library/bin;" + $env:PATH
& $playCTest --test-dir build/play-cmake --output-on-failure --parallel 4
```

On another PC use installed CMake/CTest, compiler and package paths. Debug keeps the existing assertion-based tests active. This module-only configuration uses `core/play/CMakeLists.txt`, without compiling the sheet engine or frontend. Both console executables and all fourteen native suites are built by CMake. The complete CTest run includes admission, persistence, codec, managers, buffered stop, sendCommand and the new PermissionSet/PermissionManager tests.

The permission model adds no dependency. Its focused suites check all 4096 pairs of the 64 permission combinations, the 4608-case delegate mutation matrix, protected host/self targets, host-only administrative changes, preset replacement, immediate revocation, detached/validated snapshots, staged-registration rollback, all ten feature collisions and concurrent calls. Session, ProtocolManager, client DTOs and archives now use PermissionManager. RoleManager source/tests were removed. JSON v3 and numeric archive v2 deliberately reject the retired formats; no migration/importer is added. See session-permissions.md.

`play_send_command_test` verifies offline host identity, queued cancellation/destruction, Session provider lifetime, shared local/remote callback serialization, current-role checks, concurrent submissions, arbitrary JSON results, out-of-order/fragmented/concatenated replies, invalid/duplicate correlation, bounded queues, five-second timeout, EOF/disconnect and stop/resume while remote work waits behind a local callback. Existing client admission coverage includes eight participants and three host reconstructions, cancellation races, malformed/partial/maximum-size frames and fixed admission deadlines.

Earlier real-process diagnostics under `build/play/client-admission/` and `build/play/client-validation/` remain optional local artifacts. They cover console token retention, host/client restarts, replacement, abrupt exits, corrupted credential files and preservation of complete admissions before unsupported following data. Current executables are those generated in `build/play-cmake/`; console syntax remains admission/monitoring only.

Loopback execution needs local socket permissions because the sandbox otherwise denies connects with WSA error 10013. CTest was run with those permissions. POSIX execution and physical two-PC networking remain unverified for this implementation. WSL Ubuntu 24.04 was previously found without the required development tools; its packages/configuration were not changed.

Historical Windows/Linux/Tailscale results and the old Linux send-timeout issue are archived in `history/2026-11-10-game-network-prototype.md`; they concern removed code.

## Dependencies

The sheet and play modules require nlohmann-json 3.12 or newer. CMake resolves `nlohmann_json::nlohmann_json`; play publicly exposes JSON in its callback API. Play also requires OpenSSL 3.0+ Crypto (`OpenSSL::Crypto`) for random tokens and SHA-256, linked privately alongside `Threads::Threads` and, on Windows, `ws2_32`. The backend and all play tests are native C++; Python is not required by their CMake targets. Local verification used the existing OpenSSL 3.0.15 headers, import library, and runtime DLL without installing packages.

The CMake JSON package is installed locally under ignored `build/play/dependencies/json-install/`. Its official source is `https://github.com/nlohmann/json/tree/v3.12.0`. The earlier single-header diagnostic cache under `build/play/dependencies/include/` is no longer needed by the verified CMake build.

## Permission Integration Verification (2026-11-17)

The same project CMake build compiles thirteen library sources, both tools and fourteen native suites. All fourteen pass through CTest, including all 4096 dispatch permission pairs, explicit Any versus authenticated zero, unknown bits/access enums, immediate grants/revokes, strict client masks and arbitrary saved masks 0/18/32/63. Existing trusted-context, replacement, cancellation, queue and buffered-stop regressions pass.

The optional ignored build/play-cmake/permission_process_smoke.py uses the CMake-built tools as separate processes. It verifies join/resume, replacement, retained original token files, host-process restart with custom mask 18, a second participant, JSON v2 rejection before identity issuance and version-1 archive rejection without overwrite. It requires local TCP permissions, like native network tests. No manual compiler invocation is used. Live permission notifications and mutation RPCs remain deferred.
