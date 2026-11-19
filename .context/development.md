# Development

## Required Project Build Entry Point

The user requested on 2026-11-17 that project compilation use the repository-root CMakeLists.txt with build as its binary directory. Root adds core, which adds sheet and play. With Ninja, generated play binaries therefore belong to build/core/play. No output-directory override or CMake source change is needed.

Configure from the repository root using an available CMake 3.25+ and Ninja:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
    cmake --build build --parallel 4
    ctest --test-dir build --output-on-failure --parallel 4

This root build includes the sheet and play modules and their tests. Debug keeps assertion-based suites active. Supply the compiler/package settings required by the local environment without changing the -S . -B build layout. A single-configuration generator keeps play outputs directly in build/core/play; multi-configuration generators may add a configuration directory.

When only the two play tools need rebuilding, keep the same root configuration and select their targets:

    cmake --build build --target golarion_play_host golarion_play_client --parallel 4

After the play test targets have also been built, their subset can be run with:

    ctest --test-dir build/core/play --output-on-failure --parallel 4

Do not switch normal project work to -S core/play or build/play-cmake. The module remains independently compilable, but the agreed project workflow uses the root. Do not introduce manual compiler/linker commands.

## Dependencies and Local Tools

C++20 is required. Both modules use nlohmann_json 3.12+. Play also uses OpenSSL Crypto 3.0+, Threads and ws2_32 on Windows. The root configures the sheet module too, including its Python resource-generation dependency. Ensure runtime DLLs are reachable on Windows.

The root build is now verified with GCC 10.3, Ninja, CMake 3.31.10, JSON 3.12.0 and existing OpenSSL 3.0.15. Local CMake/CTest are under ignored build/_tools/cmake/cmake/data/bin; JSON is installed as a real CMake package under build/_deps/json-install. The historical standalone setup is archived separately.

Verified Windows commands from the repository root:

    $playCMake = "./build/_tools/cmake/cmake/data/bin/cmake.exe"
    $playCTest = "./build/_tools/cmake/cmake/data/bin/ctest.exe"
    $playJsonPrefix = (Resolve-Path "build/_deps/json-install").Path
    & $playCMake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=C:/TDM-GCC-64/bin/g++.exe -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON "-DCMAKE_PREFIX_PATH=$playJsonPrefix" -DOPENSSL_ROOT_DIR=C:/Users/Daniele/miniconda3/Library -DOPENSSL_CRYPTO_LIBRARY=C:/Users/Daniele/miniconda3/Library/lib/libcrypto.lib
    $playTargets = @("golarion_play_host", "golarion_play_client") + @(Select-String -Path "core/play/CMakeLists.txt" -Pattern 'add_executable\((play_[a-z_]+_test)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    & $playCMake --build build --target @playTargets --parallel 4
    $env:PATH = "C:/Users/Daniele/miniconda3/Library/bin;" + $env:PATH
    & $playCTest --test-dir build/core/play --output-on-failure --parallel 4

The focused run configures the root graph and builds both play tools plus all play tests; it does not claim a full sheet-test run. CMakeCache confirms CMAKE_HOME_DIRECTORY is the repository root and the build type is Debug. Adapt compiler/package paths on another PC without changing -S . -B build. Local TCP tests require socket permissions in this environment.

## Host and Client Tools

Expected Windows executable locations after the root build:

    ./build/core/play/golarion_play_host.exe 127.0.0.1 42500 ./session-identities.json "La Città Sommersa"
    ./build/core/play/golarion_play_client.exe 127.0.0.1 42500 A7K2Q ./my-session-token.txt

Run separate terminals and replace the code with the one printed by the host. Host syntax is IPv4/port/[identity archive]/[session name]. The optional name follows an explicit archive path and appears in the desktop client. Port zero requests an OS-assigned port. Client syntax is IPv4/port/code/[token file]. Linux names omit .exe. For LAN/VPN bind the host appropriately and use its reachable address on clients.

The host creates/loads its archive before listening; default is session-identities.json in its working directory. Client default is client-token.txt: missing selects join, existing valid selects resume. One token file belongs to one participant/session. JSON protocol is v3; identity archives are v2 with hostId, numeric permission masks and token hashes. Legacy formats are rejected without conversion or automatic deletion. Use fresh client credentials after discarding an archive. The tools configure no firewall/router rules.

## Verified Commands, Notifications and Presence (2026-11-19)

The root CMake build compiles fifteen play library sources, both tools and seventeen native suites to build/core/play. All seventeen pass on Windows. permission_commands_test covers RPC access/schema/transactions; play_events_test covers targeted live masks, fragmented/mixed frames, no-op/failed-save suppression, fixed deadlines, 64-frame/256-KiB queue limits, stale contexts and stop/replacement races. play_participants_test additionally covers connected-only projections, initial snapshots, replacement, failed admission, zero-permission access, revisions, host reconstruction, frame limits and concurrent presence changes. Existing framing, queue, identity and buffered-stop regressions pass.

Permission read/mutation commands and targeted live notifications are complete for this step. ClientSnapshot refreshes on permissions.changed; permissions.get supplies current server data independently. Permanent tests live in core/play/tests; no frontend or mutation CLI prompt was added. Historical standalone/process checks are in history/2026-11-17-standalone-play-build.md. POSIX/physical two-PC verification of this step remains pending.

The final presence verification used a clean root CMake build and three repeat runs of play_participants_test before the complete suite. An earlier incremental-executable failure and its diagnostic evidence are recorded separately in history/2026-11-19-presence-build-verification.md. Connected-participant contracts and Views are documented in session-participants.md.

## Session Snapshot Verification

The root GCC 10.3 build and Qt/MSVC build compile. The controller suite passes. In the root CTest run, 16 of 17 suites pass and `play_participants_test` exits while rethrowing the expected rejected-RPC exception through a future (Windows exception code `0x20474343`). The same participant suite passes when compiled with MSVC against the Qt build's play library, including the active and stopped session view checks.
