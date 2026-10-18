# Interoperability and Layout

Read this document when changing the C boundary, build targets, frontend boundary, or source-tree organization.

- A minimal C-compatible API creates and destroys opaque character-sheet handles.
- Further C API design is deferred until the C++ model is complete; current model additions are not exposed through FFI.
- CMake builds the model, shared interoperability library, and smoke test.
- Frontend work is explicitly deferred.
- Backend headers, implementations, and tests mirror the logical areas `character`, `resource`, `view`, `data`, `persistence`, `util`, and `ffi`; modifiers and contributions both live under `resource`, while all GUI snapshot structs are collected under `include/golarion/view` and use the `View` suffix.
