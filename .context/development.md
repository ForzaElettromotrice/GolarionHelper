# Development

## Required Project Build Entry Point

The user requested on 2026-11-17 that project compilation use the repository-root CMakeLists.txt with build as its binary directory. Root adds core, which adds sheet and play. With Ninja, generated play binaries therefore belong to build/core/play. No output-directory override or CMake source change is needed.

Configure from the repository root using an available CMake 3.25+ and Ninja:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
    cmake --build build --parallel 4
    ctest --test-dir build --output-on-failure --parallel 4

This root build includes the sheet and play modules and their tests. Debug keeps assertion-based suites active. Supply the compiler/package settings required by the local environment without changing the -S . -B build layout. A single-configuration generator keeps play outputs directly in build/core/play; multi-configuration generators may add a configuration directory.

To rebuild just the play library, keep the root configuration and build the `golarion_play` target.

    ctest --test-dir build/core/play --output-on-failure --parallel 4

Do not switch normal project work to -S core/play or build/play-cmake. Select play without Qt using -DGOLARION_TARGET=golarion_play from the root. Do not introduce manual compiler/linker commands.

## Dependencies and Local Tools

Official catalogs are runtime JSON data. CMake copies the files listed by `core/sheet/CMakeLists.txt` to the build catalog directory and installs those same files with the Runtime component. The generated headers contain only file lists. The desktop resolves the directory relative to its executable: `catalogs/` beside the executable on Windows and in development builds, and `share/golarion-helper/catalogs/` under the installation prefix on Linux. Core tests default to the configured build catalog directory. Catalog loading and indexing use the existing C++ JSON library without Qt or Python.

The desktop executable is always under `<build>/frontend/app/` (with `.exe` on Windows), including with multi-configuration generators. Use separate build trees to retain both Debug and Release executables.

On native Arch x86-64 the desktop `installer` target runs makepkg on the Runtime install component, including the desktop entry and asset credits. It writes `build/packages/golarion-helper-<version>-1-x86_64.pkg.tar.zst` without installing it. The recipient uses pacman; the Qt dependency minimum tracks the minor version used to build the binary.

Native Windows desktop builds expose `installer` (use Release): it depends on the app and `prepare_nsis`, then runs CPack's NSIS generator on the Runtime install component. Output: `build/packages/GolarionHelper-<version>-Setup.exe`. NSIS discovery prefers a system installation, then a cached kit, otherwise downloads pinned 3.13 under `build/_deps/nsis/` if allowed. Plain installation never prepares NSIS: on Windows it stages the executable, Qt/QML runtime and compiler runtime; on Linux it installs the executable for use with system Qt.

Only the frontend discovers Qt (minimum 6.9). If missing and downloads are enabled, CMake fetches the SHA-256-pinned public Qt 6.9.3 kit on native Linux x86-64 or Windows MSVC 2022 x64. It stages extraction before publishing the kit under `build/_deps/qt/6.9.3/<kit>/`; subsequent offline builds can reuse it. Archive revisions, destinations and hashes are in `cmake/Qt693Archives.cmake`.

For targets using JSON, the root first searches for an installed nlohmann_json 3.12+ CMake package. If unavailable, it fetches the SHA-256-pinned 3.12.0 release into the build tree. Configure with `-DGOLARION_ALLOW_DOWNLOADS=OFF` to require a local package or a previously populated source tree. The UI-only target skips JSON; Qt is still an SDK dependency.

C++20 is required. Both modules use nlohmann_json 3.12+. Play uses Threads, plus ws2_32 and bcrypt on Windows. The root configures the sheet module too, including its Python resource-generation dependency.

The root build is now verified with GCC 10.3, Ninja, CMake 3.31.10, JSON 3.12.0 and existing OpenSSL 3.0.15. Local CMake/CTest are under ignored build/_tools/cmake/cmake/data/bin; JSON is installed as a real CMake package under build/_deps/json-install. The historical standalone setup is archived separately.

Verified Windows commands from the repository root:

    $playCMake = "./build/_tools/cmake/cmake/data/bin/cmake.exe"
    $playCTest = "./build/_tools/cmake/cmake/data/bin/ctest.exe"
    $playJsonPrefix = (Resolve-Path "build/_deps/json-install").Path
    & $playCMake -S . -B build -DGOLARION_TARGET=golarion_play -G Ninja -DCMAKE_CXX_COMPILER=C:/TDM-GCC-64/bin/g++.exe -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON "-DCMAKE_PREFIX_PATH=$playJsonPrefix"
    $playTargets = @("golarion_play") + @(Select-String -Path "core/play/CMakeLists.txt" -Pattern 'add_executable\((play_[a-z_]+_test)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    & $playCMake --build build --target @playTargets --parallel 4
    & $playCTest --test-dir build/core/play --output-on-failure --parallel 4

The focused commands configure the root graph and build the play library and its tests. Adapt compiler/package paths on another PC without changing -S . -B build. Local TCP tests require socket permissions in this environment.

For manual host/client checks, use the desktop app. The former diagnostic CLI sources and targets have been removed; see [historical notes](history/2026-11-23-play-cli-removal.md).

## Verified Commands, Notifications and Presence (2026-11-19)

The verification run on Windows passed seventeen native play suites from build/core/play. permission_commands_test covers RPC access/schema/transactions; play_events_test covers targeted live masks, fragmented/mixed frames, no-op/failed-save suppression, fixed deadlines, 64-frame/256-KiB queue limits, stale contexts and stop/replacement races. play_participants_test additionally covers connected-only projections, initial snapshots, replacement, failed admission, zero-permission access, revisions, host reconstruction, frame limits and concurrent presence changes. Existing framing, queue, identity and buffered-stop regressions passed.

Permission read/mutation commands and targeted live notifications are complete for this step. ClientSnapshot refreshes on permissions.changed; permissions.get supplies current server data independently. Permanent tests live in core/play/tests; no frontend or mutation CLI prompt was added. Historical standalone/process checks are in history/2026-11-17-standalone-play-build.md. POSIX/physical two-PC verification of this step remains pending.

The final presence verification used a clean root CMake build and three repeat runs of play_participants_test before the complete suite. An earlier incremental-executable failure and its diagnostic evidence are recorded separately in history/2026-11-19-presence-build-verification.md. Connected-participant contracts and Views are documented in session-participants.md.

## Session Snapshot Verification

Display-name verification used root CMake configurations on Windows. GCC 10.3 passed all 17 play suites in build/core/play. MSVC with Qt 6.8.3 built the desktop application and passed all 17 play suites plus the Qt session controller suite. Tests cover v2 archive upgrade with the original token, v3 reload, accented and duplicate names, a rename published during socket replacement, and rollback on a failed archive write.
