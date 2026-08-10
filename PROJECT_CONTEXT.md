# GolarionHelper Context

## Goal

Build a local-first Pathfinder 1e character-sheet engine in C++, kept independent from its future portable frontend.

## Current State

- Development is taking place on the orphan branch `cpp-rebuild`.
- The previous Java model rebuild is preserved on `model-rebuild` at commit `e99d776`.
- The project currently contains only a minimal C++20 backend.
- `CharacterSheet` owns and initializes one `ResourceManager` before registering its model components. It also owns `Skills` and exposes the supported skill mutations without exposing model getters. Removing a user-defined specialization also unregisters its resource and removes every modifier-group entry targeting it; affected groups survive when other modifiers remain and are destroyed when empty. Canonical specializations cannot be removed.
- `CharacterSheet` owns `Movement` and a `MovementGroupManager`. It exposes creation, destruction, and enable/disable operations for user movement groups; newly created groups start disabled. `Movement` starts with a provisional structural racial land-speed grant of 6 movement units (9 m), which race modeling will eventually replace and rebuild at startup.
- Movement uses integer units of 1.5 m and supports land, climb, swim, burrow, and fly modes. Fly grants carry maneuverability (clumsy, poor, average, good, or perfect) and its Pathfinder Fly-check modifier.
- Every movement grant is an independent, atomic profile identified by a stable ID and source. Multiple grants of the same type remain separate alternatives rather than merging speed and properties field by field.
- Each grant registers an enhanceable resource beneath `speed.all`, `speed.<type>`, and `speed.<type>.<grantId>`. Additive speed bonuses therefore reuse normal modifier stacking at global, movement-type, or exact-grant scope.
- `MovementAdjustment` handles non-additive transformations selected globally, by movement type, or by exact grant: percentage speed multipliers, absolute speed limits, maneuverability shifts, and movement blocks. Conditional adjustments are resolved for display but do not alter the permanent result.
- A grant cannot be removed while an exact-grant adjustment or a direct modifier still targets it. Dependents must be removed first, preventing one group from silently mutating another. `MovementView` exposes every independently resolved profile, its modifier snapshot, applicable adjustments, effective speed, maneuverability, and usability.
- `MovementGroup` is a passive, non-empty collection containing grants, adjustments, or both. Grant and adjustment IDs must be unique within their respective part of the group.
- `MovementGroupManager` owns user-created movement groups and applies them through the typed `ResourceManager` collections. Activation adds grants before adjustments; deactivation removes adjustments before grants. Both directions use rollback, group snapshots are deterministically ordered, and enabled state is persisted.
- `CharacterSheet` owns a `ModifierGroupManager` after its `ResourceManager` and exposes creation, destruction, and enable/disable operations for modifier groups; newly created groups start disabled.
- `CharacterSheet` also owns a `ContributionGroupManager` and exposes the corresponding creation, destruction, and enable/disable operations; newly created contribution groups start disabled.
- The six abilities start at 10, validate positive base values, and register an enhanceable resource plus total/modifier targets using `str`/`strMod`, `dex`/`dexMod`, `con`/`conMod`, `int`/`intMod`, `wis`/`wisMod`, and `cha`/`chaMod`.
- `SkillType` defines the standard Pathfinder skills and their fixed metadata: display label, resource name, default ability, trained-only status, specialization requirement, and armor-check-penalty applicability.
- `Skill` models one concrete skill entry, including specialized Craft/Perform/Profession entries, and registers only an enhanceable resource. Skills are deliberately not expression targets; their totals combine the current ability modifier, ranks, the trained class-skill bonus, and their `ResourceManager` modifiers.
- `Skills` owns all non-specialized standard skills and Craft/Perform/Profession entries. It creates and registers the canonical specializations at initialization, allows additional user-defined specializations, assigns every concrete skill all applicable independent modifier categories, and preserves category-level class-skill and ability defaults for specializations created later. Canonical specializations remain present in the model; eventual visibility filtering belongs to the GUI.
- `SkillView` exposes identity, display name, ability and class-skill contributions, ranks, usability, total value, resolved modifiers, and whether the entry is custom. `SkillsView` provides every concrete skill in deterministic type/specialization order and is included in `CharacterSheetView`.
- `SkillSaveData` stores only source state for one concrete skill; `SkillsSaveData` stores the complete deterministic skill collection. Skill loading recreates custom resources before modifier groups and validates built-in completeness, identity uniqueness, canonical/custom classification, and category-default consistency.
- `SavingThrow` models Fortitude, Reflex, or Will with a non-negative base value and a replaceable associated ability; defaults are Constitution, Dexterity, and Wisdom. `SavingThrows` owns all three and registers `savingThrow.all` plus the concrete resources so global and per-save modifiers compose through inheritance.
- `SavingThrowView` exposes the base, selected ability contribution, permanent total, and conditional modifier information. Saving-throw source state is persisted in `SavingThrowSaveData`/`SavingThrowsSaveData`, included in `CharacterSheetView`, and editable through `CharacterSheet`.
- `Initiative` registers the `initiative` enhanceable resource and calculates its total from the selected ability modifier plus resource modifiers. It defaults to Dexterity, supports permanent ability replacement, is exposed through `InitiativeView`, and persists its selected ability in `InitiativeSaveData`.
- `HitPoints` stores base maximum, lethal damage taken, temporary hit points, and non-lethal damage. Its effective maximum adds the accumulated `hp.max` contributions and never drops below zero; current hit points are derived as `max - damageTaken`, so maximum changes automatically adjust current hit points while preserving wounds. `hp.max` is an accumulated resource, while none of the hit-point values are expression targets.
- `CharacterSheet` registers `level` as an expression target that currently always resolves to 1, then installs the structural `hitPoints.constitution` contribution on `hp.max` with expression `@conMod * @level`. This contribution is rebuilt at startup and is not persisted. Character level is a temporary approximation for Hit Dice; racial Hit Dice and the minimum of 1 hit point per Hit Die remain future concerns.
- `HitPoints::damage` consumes temporary hit points first, then applies lethal damage to current hit points or accumulates non-lethal damage up to the effective maximum and converts overflow to lethal damage. `HitPoints::heal` restores current hit points up to the effective maximum and removes the same amount of non-lethal damage without restoring temporary hit points.
- Temporary hit points are stored as independent source pools rather than one integer. Each pool has a user-provided normalized ID, remaining points, and an optional `GameDuration`; different IDs stack, while reapplying the same ID keeps the stronger remaining amount and, on a tie, the longer duration. Damage consumes pools by shortest remaining duration first, indefinite pools last, and expiration removes remaining points without transferring damage.
- `GameDuration` stores non-negative game time in rounds and provides checked factories for rounds, minutes, and hours. `CharacterSheet::advanceTime` currently advances temporary-hit-point durations and can later be extended to other timed effects.
- `HitPointsView` exposes all four sheet values, individual temporary-hit-point pools, and the resolved maximum contribution set. `HitPointsSaveData` persists damage taken and temporary pools rather than derived totals, while maximum contributions remain owned and persisted by contribution groups. `CharacterSheet` exposes maximum/current/non-lethal setters, temporary-pool addition/removal, healing, typed damage, and time advancement.
- The first modifier-domain slice is implemented: modifier enums, stacking metadata, immutable modifier data, UUID generation, normalization, and constructor invariants.
- `ResourceManager` owns the `ModifierSet` of every registered enhanceable resource and keeps live integer targets in a separate normalized registry. It routes modifier additions/removals, calculates totals by resource name, and detects circular target resolution.
- `ResourceManager` supports typed collection resources as its third resource category. A component registers a normalized resource name, its item type, and add/remove callbacks; callers add strongly typed values through the global manager, while an incompatible item type is rejected before dispatch. Collection resource names are included in `ResourceManagerView`.
- `Movement` registers `movement.grants` for `MovementGrant` values and `movement.adjustments` for `MovementAdjustment` values. `HitPoints` registers `hp.temporary` for `TemporaryHitPointGrant` values; the grant amount is an integer expression resolved through the manager when added. The component remains the owner of its state and rules, while `ResourceManager` only validates and routes collection operations.
- `CharacterSheet` routes temporary-hit-point mutations and movement-group activation through the registered collection resources. `Movement` itself has no save root: structural grants and adjustments are reconstructed by their owning race/class/item/effect, while user-owned definitions are persisted by `MovementGroupManager`. Hit-point persistence still stores mutable remaining temporary pools. The collection registry is always reconstructed at startup.
- `ResourceManager` also supports accumulated resources backed by `ContributionSet`. Each `Contribution` has an explicit normalized ID and an integer expression; all contributions stack, may resolve to signed values, and can be added or removed independently without modifier types, conditions, or stacking rules. `HitPoints` uses this mechanism for its effective maximum.
- `ContributionGroup` is a passive, non-empty collection of targeted contributions intended to represent one user-owned source spanning one or more accumulated resources. It normalizes resource names and requires contribution IDs to be unique within the group.
- `ContributionGroupManager` owns contribution groups, tracks their enabled state, and applies or removes their targeted contributions through `ResourceManager`. Activation and deactivation roll back already processed contributions on failure. It is connected to `CharacterSheet` and included in its GUI and persistence snapshots.
- Enhanceable resources may inherit from already registered parent resources. Totals and views combine direct and inherited modifiers before applying stacking rules, deduplicate shared ancestors in multiple-inheritance graphs, and automatically expose category modifiers to resources registered later.
- Skill modifier categories are intended to remain independent: a concrete skill lists every applicable category directly (for example both `skill.all` and `skill.craft`) rather than making `skill.craft` inherit from `skill.all`.
- Integer expressions support literals, registered targets prefixed by `@`, parentheses, unary signs, whitespace, and the four basic arithmetic operators with integer division.
- `ModifierSet` calculates permanent totals through `ResourceManager`, applies Pathfinder stacking rules, caps inherent bonuses at +5, subtracts all penalties, excludes conditional modifiers, and rejects negative resolved magnitudes.
- `ModifierView` exposes every modifier field plus its resolved value. `ModifierSetView` contains the permanent total, effective conditional deltas grouped by condition, and all resolved modifier snapshots; each expression is evaluated once per set snapshot.
- `AbilityView` contains the ability type, base value, total value, derived modifier, and its complete `ModifierSetView`; `AbilityScore::toView` resolves the modifier set once per snapshot.
- `AbilitySaveData` contains only the ability type and base value; calculated values and modifier snapshots are rebuilt rather than persisted.
- `ModifierSaveData` preserves the stable modifier ID and every source field, excluding only the resolved value. `ModifierGroupSaveData` stores targeted modifiers, while `ModifierGroupManagerSaveData` stores deterministically ordered group IDs, enabled states, and group contents.
- `ContributionView` contains the ID, expression, and resolved value; `ContributionSetView` adds the accumulated total. Group and manager views expose targeted contributions and deterministically ordered group states.
- `ContributionSaveData`, `ContributionGroupSaveData`, and `ContributionGroupManagerSaveData` preserve user-owned contribution groups. There is no `ContributionSetSaveData`, because registered resource sets are reconstructed by model components.
- `CharacterSheetSaveData` is the versioned persistence root and contains ability, hit-point, initiative, saving-throw, skill, movement-group-manager, modifier-group-manager, and contribution-group-manager source data; `ResourceManager` and `Movement` have no direct save data because their registries are rebuilt by model components and enabled groups. The current format version is 10; version 8 remains loadable with no user movement groups and receives the provisional default land grant. The unreleased version 9 movement-root experiment is intentionally rejected because it could not distinguish structural from user-owned grants.
- The separate `golarion_persistence` library converts `CharacterSheetSaveData` to and from readable JSON through `nlohmann-json`; persistence enum names are stable English identifiers and do not reuse GUI labels.
- `CharacterSheet::save(path)` delegates JSON file writing to the persistence module and reports file opening or writing failures with exceptions.
- `CharacterSheet::load(path)` validates format version and the complete set of six abilities, reconstructs movement groups before modifier groups so their dynamic resources exist, then reconstructs contribution groups and restores enabled states.
- `ModifierGroup` is a passive, non-empty collection of normalized resource/modifier pairs and carries no activation state. `ModifierGroupManager` stores groups under owner-provided IDs, tracks enabled state, and applies or removes their modifiers through `ResourceManager` with rollback on failed activation.
- `ModifierGroupView` contains resolved targeted modifiers. `ModifierGroupManagerView` contains deterministically ordered group IDs, enabled states, and group snapshots.
- `ResourceManagerView` exposes deterministically ordered snapshots of registered target names/current values, collection names, and every enhanceable resource. Each enhanceable-resource entry contains its name, direct parent-resource names, and its fully resolved `ModifierSetView`, including inherited modifiers; the resource's base value remains in its owning component view.
- `CharacterSheetView` aggregates abilities, hit points, initiative, saving throws, skills, resolved movement, movement groups, the resource-manager snapshot, modifier groups, and contribution groups for the GUI.
- A minimal C-compatible API creates and destroys opaque character-sheet handles.
- Further C API design is deferred until the C++ model is complete; current model additions are not exposed through FFI.
- CMake builds the model, shared interoperability library, and smoke test.
- Frontend work is explicitly deferred.
- Backend headers, implementations, and tests mirror the logical areas `character`, `resource`, `view`, `data`, `persistence`, `util`, and `ffi`; modifiers and contributions both live under `resource`, while all GUI snapshot structs are collected under `include/golarion/view` and use the `View` suffix.

## Important Decisions

- The domain model is implemented in modern C++.
- A thin API with C linkage will be the only boundary exposed to a future frontend through FFI.
- C++ types, STL containers, ownership, and exceptions do not cross that boundary.
- Complex frontend snapshots may later be exchanged as JSON, but no serialization dependency is introduced yet.
- GUI snapshots use `View`/`toView`; future persistence structures will use `SaveData`/`toSaveData` to keep calculated presentation state separate from serialized source state.
- Character-sheet components register enhanceable resources by name; they neither create nor retain their `ModifierSet`.

## Setup

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The JSON module requires the `nlohmann-json` package (`yay -S nlohmann-json` on Arch Linux).

## Pending Work

- Review the modifier fields and C++ representation together.
- Design richer C API operations only when the corresponding model behavior exists.
- When the condition system is implemented, connect it to `HitPoints`: 0 current hit points should apply disabled, negative hit points should apply dying and unconscious, reaching the negative Constitution threshold should apply dead, non-lethal damage equal to current hit points should apply staggered, and non-lethal damage above current hit points should apply unconscious. Keep this deferred until conditions have a dedicated model rather than embedding provisional condition logic in `HitPoints`.
- Evolve `CharacterSheet::advanceTime` into the central game-time propagation mechanism for all duration-based effects, including conditions, spells, class features, item effects, and other temporary resources. It currently advances only temporary-hit-point pools; introduce a shared time/effect protocol when the first additional timed subsystem is modeled.
- Complete movement's rule pipeline when armor, carried load, and conditions are modeled. `MovementGrant` already records whether armor and load apply, but those reductions are not calculated yet. Effects with a bounded increase such as `Haste` also need an explicit final-speed stage so the +9 m enhancement bonus can be capped at twice the pre-effect normal speed without bypassing stacking rules.

## After the Character Sheet

- Decide whether conditional ability substitutions need explicit model support. A possible implementation is a signed, non-stacking delta modifier such as `-@wisMod + @chaMod`, shared by skills and saving throws. This is optional: the application may instead leave situational substitutions for the player to remember and apply at the table.
- Design the complete C API only after the C++ character-sheet model has stabilized.
- Select and build the portable frontend after the backend and C API are complete.
