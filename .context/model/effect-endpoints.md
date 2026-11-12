# Declarative endpoint effects

Read this when adding catalog effects or extending the JSON/compiler bridge. The six-point backend audit was addressed on 2026-11-12; this is not a claim that every Pathfinder rule or catalog entry is implemented.

Verification: the sheet-only build and all 85 sheet tests pass, including four new endpoint/rule/sheet-operation integration executables. No `core/play` tests are part of this change.

## Ownership and expressions

Every effect requires a local `id` and a closed `type`. `compileEffect` supplies the runtime ID and source from `EffectContext`, registers the typed entry, and returns its exact cleanup. Dependencies must be installed before consumers and removed in reverse order. Target casting, group, strike, action, routine, slot and rule IDs are explicit runtime IDs, not automatically resolved source-local references.

`CollectionEffectDefinition` groups value-only endpoint definitions; it does not expose an arbitrary JSON-to-resource escape hatch. `CastingRuleEffectDefinition` describes casting predicates with typed filters and expressions, not serialized C++ functions. Both also work as cumulative favored-class benefits. Class binding visits nested entries, benefits, requirements, routine slots and progression expressions; `@classLevel` refers to the owning class, not total character level.

Unknown enum names, malformed arrays and nonintegral/out-of-range integer fields reject at the JSON boundary. Endpoint owners retain validation of references and runtime compatibility. Expressions are evaluated when consumed; parsing does not require all referenced targets to exist yet.

## Structural effects

The complete executable JSON fixtures are in `core/sheet/tests/effect/collection_effect_test.cpp`; native schemas are in `include/golarion/effect/collection_effect.hpp` and the corresponding component headers. The 49 added structural types cover:

- Defenses: `energyResistance`, `energyResistanceAdjustment`, `energyVulnerability`, `damageReduction`, `damageReductionAdjustment`, `immunity`, `spellResistance`, `spellResistanceAdjustment`, `fastHealing`, `regeneration`. RD `bypass` is an array of alternatives, each an array of jointly required traits; `[]` means no bypass. Grants preserve applicability text, tags and stacking policy where supported by the endpoint.
- Structure/movement: `sizeReplacement`, `sizeAdjustment`, `carryingCapacityMultiplier`, `movementAdjustment`, `runAdjustment`, `movementBoundedBonus`.
- Abilities/defense/checks: `abilityReplacement`, `armorClassAbilityReplacement`, `armorClassAbilitySuppression`, `armorClassDexterityRetention`, `maximumDexterityLimit`, `armorCheckPenalty`, `skillAbilityReplacement`, `classSkillGrant`, `initiativeAbilityReplacement`, `savingThrowAbilityReplacement`, `combatManeuverAbilityReplacement`, `combatManeuverDexteritySuppression`.
- Health/conditions: `temporaryHitPoints` (expression and optional durationRounds metadata), `conditionEntry` (conditionId, severity, escalation/group/parameter options). Neither adds a time engine or resurrection protocol.
- Actions: `actionCategory`, `actionGrant`, `actionInhibition`, `actionNote`, `actionCostReplacement`.
- Attacks: `strikeGrant`, `damageComponentGrant`, `criticalAdjustment`, `attackAbilityReplacement`, `damageAbilityReplacement`, `damageDiceAdjustment`, `attackDistanceAdjustment`, `attackRequirement`, `attackDefenseReplacement`, `strikeWeaponWeightAdjustment`, `strikeUsageAdjustment`.
- Routines: `attackRoutine`, `routineProgressionGrant`, `routineAttackBonusAdjustment`. Slots preserve selectors, hand roles, optional assignment and progression; assignment requirements remain structural, not expression requirements.

The existing modifier, contribution, base-size/body, movement/language/feat/spell grants and favored-class effects remain available. Internally owned bookkeeping endpoints such as current carrying size and inventory weight are not duplicated by catalog sources.

Repertoire sources additionally use `spellRepertoireQuota`, `spellRepertoireGrant` and `spellRepertoireList`; schemas and lifetime rules are in [repertoire.md](repertoire.md).

## Casting rules and feat prerequisites

Each casting rule requires `castingId`. Examples and lifecycle tests are in `core/sheet/tests/effect/casting_rule_effect_test.cpp`.

| Type | Rule data |
| --- | --- |
| `spellLevelOverride` | groupId, spellId, level (0–9) |
| `preparationCost` | school, requiredSlots |
| `preparationCostExemption` | ruleId |
| `preparationCheck` | expression; optional filter |
| `preparationCheckExemption` | checkId; optional expression (default 1), filter |
| `consumptionExemption` | optional expression (default 1), filter |
| `spontaneousConversion` | explicit nonempty groupIds and replacements `{spellId, level}`; optional preparedFilter |
| `metamagicGrant` | phase, metamagic; optional elementalEnergy, slotCost, rodGrade, castingScope, filter, poolId, uses |
| `metamagicMixingPermission` | optional filter, poolId, uses |

Spell filters intersect ID and school restrictions; an omitted/empty dimension imposes no restriction. A preparation check only constrains matching spells. Exemptions require both a match and a true expression. Conversion sources explicitly authorize every group of a linked preparation. Filtered metamagic grants can explicitly certify applicability not expressible in spell metadata; authors must supply valid eligibility data. Without such certification the native applicability safeguards remain in force.

`poolId` names an existing source/item usage pool resolved through EffectContext. Adapters share the original balance, aggregate payment by underlying pool, never refill on registration and do not duplicate item persistence. This schema does not invent daily pools for standalone feats; providers may still own/register those natively.

`featPrerequisiteAlternative` takes target, expression and the existing any/type/ID feat filter vocabulary. Its calculation reads the original target resolver; candidates remain alternatives, not replacements of the real sheet value. Arbitrary choice-sensitive native callbacks are not serializable; this declarative schema supports only its stated filters. Prerequisite exemptions remain above the value registry.

## Boundaries

Catalog migration/population, arbitrary custom callback logic, spell targeting/resolution, turn/time tracking, physical hand occupancy and the complete C API are separate work. JSON support does not install concrete class/feat/item mechanics automatically. No persistence format bump is needed: source configuration reconstructs effects; existing Data stores mutable player state.
