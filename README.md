# GolarionHelper 1.0.0

Local-first tabletop role-playing application, expanding toward a virtual tabletop.

The C++ backend provides the Pathfinder 1e character-sheet engine and an initial TCP host/client prototype for shared game presence. The desktop frontend and further play features will be introduced later.

Version 1.0.0 marks the first character-sheet backend release: C++ model, save data, Views and custom items/spells/feats/enhancements, including free-text sheet notes. Save schema version 41 is independent of the software version; supported older saves load with empty notes.

Complete rules datasets, frontend editing, full C/FFI bindings and completion of the play module are outside this milestone. See the [Data/View audit](.context/model/backend-v1.md).

## Structure

- `core/`: backend modules and their build entry point.
- `core/sheet/`: the character-sheet engine, including public headers, implementation, catalogs, persistence, C API, and tests.
- `core/play/`: independent C++20 live-presence model, TCP host/client backend, diagnostic console tools, and tests.

The existing `golarion_model`, `golarion_persistence`, and `golarion` build targets and `golarion/...` include paths are unchanged.

Play targets are `golarion_play`, `golarion_play_host`, and `golarion_play_client`. See [development instructions](.context/development.md#standalone-play-backend) for standalone compilation and a multi-process connection test.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

