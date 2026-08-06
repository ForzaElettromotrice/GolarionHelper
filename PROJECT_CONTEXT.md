# GolarionHelper Context

## Goal

Build a local-first Pathfinder 1e character-sheet engine in C++, kept independent from its future portable frontend.

## Current State

- Development is taking place on the orphan branch `cpp-rebuild`.
- The previous Java model rebuild is preserved on `model-rebuild` at commit `e99d776`.
- The project currently contains only a minimal C++20 backend.
- `CharacterSheet` is intentionally empty.
- A minimal C-compatible API creates and destroys opaque character-sheet handles.
- CMake builds the model, shared interoperability library, and smoke test.
- Frontend work is explicitly deferred.

## Important Decisions

- The domain model is implemented in modern C++.
- A thin API with C linkage will be the only boundary exposed to a future frontend through FFI.
- C++ types, STL containers, ownership, and exceptions do not cross that boundary.
- Complex frontend snapshots may later be exchanged as JSON, but no serialization dependency is introduced yet.

## Setup

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Pending Work

- Review the minimal structure together.
- Rebuild the character-sheet model incrementally, starting from the first explicitly selected domain component.
- Design richer C API operations only when the corresponding model behavior exists.

