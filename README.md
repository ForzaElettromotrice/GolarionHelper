# GolarionHelper 1.1.0

Local-first tabletop role-playing application, expanding toward a virtual tabletop.

The C++ backend provides the Pathfinder 1e character-sheet engine and an initial TCP host/client prototype for shared game presence. The Qt Quick desktop app uses the C++ backend directly.

Version 1.1.0 adds generic, group-activated enhancements and spell-cast results with target limits and caster-evaluated effects to the character-sheet backend. Applying exported effects to recipients belongs to the game session's sheet manager. Save schema version 42 is independent of the software version; older per-entry enhancement groups are unsupported; unrelated existing compatibility paths remain. Notes default to empty in supported pre-note saves.

Complete rules datasets and completion of the play module are outside this milestone. See the [Data/View audit](.context/model/backend-v1.md).

## Structure

- `core/`: backend modules and their build entry point.
- `core/sheet/`: the C++ character-sheet engine, including public headers, implementation, catalogs, persistence, and tests.
- `core/play/`: independent C++20 live-presence model, TCP host/client backend, and tests.
- `frontend/`: the desktop application, reusable sheet panels and their tests.

The `golarion_model` build target contains the sheet model and JSON persistence. Public C++ headers use `golarion/...` include paths.

The `golarion_play` library provides both session modes to the desktop application. See [development instructions](.context/development.md) for building and running tests.

## Build and test

CMake uses installed nlohmann_json 3.12+ and Qt 6.9+ packages when available. Missing dependencies can be downloaded under `build/_deps/` with pinned checksums. The frontend's [Qt fallback](frontend/README.md) uses Qt 6.9.3 on Linux x86-64 and Windows MSVC x64. For offline builds use `-DGOLARION_ALLOW_DOWNLOADS=OFF`.

```sh
cmake -S . -B build
cmake --build build --parallel
```

The default is the desktop application (`golarion_helper`) and its dependencies. To configure a smaller build, pass `-DGOLARION_TARGET=golarion_model` (sheet only), `golarion_play` (play and sheet), or `golarion_sheet_ui` (Qt panels only). Pass `-DGOLARION_TARGET=golarion_helper` to switch back in the same build directory. Tests are opt-in: configure with `-DBUILD_TESTING=ON`, build, then run `ctest --test-dir build --output-on-failure`.

See the [frontend instructions](frontend/README.md) for panel contracts and Windows build notes.

## Windows installer

From a native Windows desktop build with Visual Studio:

```sh
cmake --build build --config Release --target installer
```

This builds the app, prepares NSIS if needed and writes `build/packages/GolarionHelper-<version>-Setup.exe`. For single-configuration generators such as Ninja, configure with `-DCMAKE_BUILD_TYPE=Release` first. To stage the app and its Qt runtime for testing without creating an installer, build the app first and run `cmake --install build --config Release --prefix build/stage --component Runtime`. Staging does not need NSIS.

## Arch Linux package

On native Arch Linux x86-64 the same `installer` target uses `makepkg` (requires the standard `base-devel` tools). Configure a Release desktop build, then run:

```sh
cmake --build build --config Release --target installer
```

Share `build/packages/golarion-helper-<version>-1-x86_64.pkg.tar.zst`. The recipient installs that file with `sudo pacman -U ./<filename>`. Qt is a system dependency resolved by pacman, not bundled. The package adds an application menu entry; it does not include or modify player saves.

