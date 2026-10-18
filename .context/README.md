# GolarionHelper Context

## Project Goal

Build a local-first Pathfinder 1e character-sheet engine in C++, independent from its future portable frontend.

## Current State

- Development is on `cpp-rebuild`, synchronized with `origin/cpp-rebuild`; the previous Java rebuild remains preserved on `model-rebuild` at `e99d776`.
- The C++20 backend, persistence, Views, and a minimal opaque-handle C API are in place; frontend work remains deferred.
- Declarative `EffectDefinition` values and `compileEffect` are shared by races and items.
- `RaceDefinitionManager` loads the embedded race catalog, while the per-character `Race` runtime applies effects, manages alternate features and choices, persists configuration, and exposes `RaceView`. The initial catalog contains Umano.
- `FeatDefinition` describes feats declaratively, but no feat catalog, runtime manager, View, or persistence exists yet.
- Informational special defenses cover the planned categories, including the locally implemented energy vulnerabilities.

## Next Steps

Continue in small, separate changes:

1. Expand declarative effect alternatives only when another canonical race requires them.
2. Add the catalog View needed to choose a race and its alternate features.
3. Add and review further canonical race definitions one at a time.

## Context Structure

- [`model/README.md`](model/README.md): index of model subsystems. Start there, then read only the document relevant to the current change.
- [`architecture.md`](architecture.md): cross-cutting architectural decisions. Read before changing ownership, persistence, Views, or the C boundary.
- [`development.md`](development.md): local build and test commands and required packages.
- [`roadmap.md`](roadmap.md): pending model work and deferred post-character-sheet work. Read when selecting or planning the next implementation step.

Useful superseded decisions and past operational events belong in `history/` when such history first needs to be retained.
