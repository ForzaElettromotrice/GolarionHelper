# Interoperability and Layout

Read this document when changing build targets, the frontend boundary, or source-tree organization.

- The Qt desktop controller links directly to the C++ sheet model. There is no C ABI; the old opaque-handle wrapper and its smoke test were removed because no project consumer used them.
- The sheet model and its JSON persistence share the `golarion_model` target. Persistence sources remain under `src/persistence`.
- Backend headers, implementations, and tests mirror the logical areas `character`, `resource`, `view`, `data`, `persistence`, and `util`; modifiers and contributions both live under `resource`, while GUI snapshot structs use the `View` suffix.
