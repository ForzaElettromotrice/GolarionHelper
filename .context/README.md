# GolarionHelper Context

## Project Goal

Build a local-first tabletop role-playing application, expanding toward a virtual tabletop in the style of Roll20. The implemented rules engine remains Pathfinder 1e; support for another ruleset is not implied.

## Current State

- Development is on `main`, the default GitHub branch, tracking `origin/main`. The previous `master` branch is retained separately.
- The C++20 backend, persistence, Views, and a minimal opaque-handle C API are in place; frontend work remains deferred.
- `core/` groups backend modules. The complete character-sheet engine now lives in `core/sheet/`, including headers, sources, catalogs, persistence, C API, and tests. Future play modules have not been designed or implemented yet.
- Declarative `EffectDefinition` values and `compileEffect` are shared by races and items.
- `RaceDefinitionManager` loads the embedded catalog of all seven core races, while the per-character `Race` runtime applies effects, manages alternate features and choices, persists configuration, and exposes `RaceView`. Standard traits use supported effects and explicit situational bonuses; unsupported mechanics remain reminders. Umano also retains Doppia Dote.
- `FeatDefinitionManager` loads immutable feat definitions from embedded JSON. `CharacterSheet` owns `Feats` with source-owned slots, prerequisite alternatives/exemptions, repeatability, choice-matched dependencies, and stable-state activation. Sheet changes automatically recheck prerequisites; manual text requirements are informational. Format 25 persists selection IDs and choices; Views expose distinct acquisitions and slots. The minimal catalog remains Iniziativa Migliorata.
- Informational special defenses cover the planned categories, including the locally implemented energy vulnerabilities.

## Next Steps

Continue in small, separate changes:

1. Add declarative support for the documented core-race gaps in small, separate changes.
2. The planned feat infrastructure is implemented. Future sheet mutation APIs must maintain the feat recheck convention.
3. Keep feat catalog expansion and catalog Views deferred. The race catalog View and further alternate racial features/races remain roadmap items, to address one at a time.

## Context Structure

- [`model/README.md`](model/README.md): index of model subsystems. Start there, then read only the document relevant to the current change.
- [`architecture.md`](architecture.md): cross-cutting architectural decisions. Read before changing ownership, persistence, Views, or the C boundary.
- [`development.md`](development.md): local build and test commands and required packages.
- [`roadmap.md`](roadmap.md): pending model work and deferred post-character-sheet work. Read when selecting or planning the next implementation step.

Useful superseded decisions and past operational events belong in `history/` when such history first needs to be retained.
