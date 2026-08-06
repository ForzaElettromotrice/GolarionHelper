# GolarionHelper

Local-first Pathfinder 1e character-sheet engine.

The project currently contains only the C++ backend. The portable frontend will be introduced later.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

