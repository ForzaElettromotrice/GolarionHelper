# Character-sheet backend 1.0.0

## Current release: 1.1.0 (2026-11-13)

Project/library version is now 1.1.0; SOVERSION remains 1 and save schema is 42. This release adds generic group-only enhancements with provenance and reversible ordered effects, plus spell consumption results carrying target limits, caster-frozen effect definitions and optional metamagic. Export errors do not refund slots or uses. See [enhancements.md](enhancements.md) and [spells.md](spells.md).

The sheet-side work is complete for this feature: recipient application belongs to the game session's sheet manager. All 94 sheet tests pass; play tests are excluded. The audit below records the historical 1.0.0 milestone, not the current enhancement schema.

## Historical 1.0.0 audit

Read for the release boundary and the Data/View coverage review performed on 2026-11-12. This records the 1.0.0 milestone at save format 41; the subsequent group-only generic enhancement change uses format 42 and is documented in [enhancements.md](enhancements.md).

## Scope

Version 1.0.0 closes the first C++ character-sheet backend milestone. The root CMake project declares the software version; the shared `golarion` library uses VERSION 1.0.0 and SOVERSION 1. This is separate from save schema 41.

The only added user feature in this release step is free-text notes: `CharacterSheet::setNotes(std::string)`, private string storage, and `notes` in both `CharacterSheetSaveData` and `CharacterSheetView`. No new subsystem, formatting engine, validation of prose or mechanical effects are introduced. Empty text clears the notes; whitespace, newlines and Unicode text are preserved. Format 41 requires a string; supported earlier formats default to empty.

## Data and View coverage

The review checked the aggregate fields, JSON mappings, reconstruction path and subsystem snapshots. No additional missing player-state field was found within the implemented sheet scope.

| Area | Saved source state | Presentation |
| --- | --- | --- |
| Identity and notes | Identity fields and verbatim notes | Detached identity and notes |
| Race | Race ID, alternate traits and choices | Active traits, selections and missing choices |
| Classes and archetypes | Class levels, archetype IDs, choices, hit dice, favored classes and rewards | Total/per-class levels, features, choices, archetypes, hit dice and favored bonuses |
| Feats | IDs, choices and user-owned extra slots | Slot origin/filter/availability, selected feats and active status |
| Abilities, skills and health | Base scores, invested ranks/specializations, damage, death and temporary pools | Derived scores, rank budget, modifiers, hit points and conditions |
| Inventory | Custom definitions, instances, choices, containers, locations, equipment and remaining charges | Item descriptions, choices, equipment, containers, money and usage pools |
| Spells | Custom definitions, repertoire provenance, prepared snapshots, consumed slots and owned/shared usage balances | Casting mode/list, repertoire sources/selections, slots, preparation and metamagic |
| Languages | Learned IDs and learning origin | Spoken languages, sources, selectable lists and remaining quotas |
| Enhancements | Group definitions and per-entry active flags | Definitions, flags and resolved values |
| Attacks and conditions | Configured attacks/assignments and manual conditions | Attack calculations, routines, strikes and active conditions |

Structural effects, grants, defenses, movement, action permissions and derived totals are rebuilt from their owners rather than serialized twice. Item-owned spell/metamagic charges remain stored only with the item. Pending choices are available through `pendingChoices()`, not saved. Missing repertoire choices block advancement.

## Verification and exclusions

Build completed successfully; all 93 sheet tests passed with assertions enabled using `ctest --test-dir build/core/sheet --output-on-failure -j 4`. No `core/play` tests were run. The generated shared library and its SONAME were checked (`libgolarion.so.1.0.0`, `libgolarion.so.1`).

- `sheet_notes_test` covers empty/default notes, exact multiline Unicode preservation, detached Views, JSON/file round trips, clearing, format-40 migration and malformed modern note values.
- The sheet suite also covers source reconstruction, custom definitions, usage pools, inactive selections, all casting modes, advancement gating and legacy saves.
- Complete production catalogs, catalog-browser Views, frontend editing, full C/FFI exposure, automatic cast targeting/resolution and completion of `core/play` are not claimed by this release. The current C boundary only creates/destroys an opaque sheet handle.
- This is a coverage review of implemented Data/View state, not proof that every Pathfinder rule or every combination of effects is implemented. The previously identified lack of a transaction around the entire level-up operation remains a separate robustness concern.
- No Git commit, push, tag or GitHub release is implied by updating the local version.
