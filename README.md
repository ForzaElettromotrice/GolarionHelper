# GolarionHelper

Local-first tabletop role-playing application, expanding toward a virtual tabletop.

The implemented C++ backend currently provides the Pathfinder 1e character-sheet engine. The portable frontend and other play modules will be introduced later; this reorganization does not add another ruleset.

## Structure

- `core/`: backend modules and their build entry point.
- `core/sheet/`: the character-sheet engine, including public headers, implementation, catalogs, persistence, C API, and tests.

The existing `golarion_model`, `golarion_persistence`, and `golarion` build targets and `golarion/...` include paths are unchanged.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

