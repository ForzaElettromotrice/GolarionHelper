# Operation Definitions

Read when changing public C++ sheet operations or constructing modifiers.

- Public operations use domain `Definition` structs for cohesive groups of arguments, not persistence `Data`. IDs that select the destination (class, casting, slot or skill type) remain separate parameters. Simple setters keep their existing signatures.
- `Modifier(ModifierDefinition)` replaces the public positional constructors. The definition carries type, source, description, bonus type, expression, optional condition/requirements and skill-rank-budget policy. Each constructed modifier still generates its own UUID and performs existing validation/normalization.
- `SpellPreparationDefinition` holds spell ID and metamagic, shared by prepared/flexible preparation. `PreparedSpellConsumptionDefinition`, `SpontaneousSpellConsumptionDefinition` and `FlexibleSpellConsumptionDefinition` hold their mode-specific cast choices. Prepared consumption defaults to an empty definition; spontaneous/flexible choices remain required. Sheet and casting instances share these types.
- `ClassOptionDefinition` holds acquisition/choice/option IDs and is used for selection and unselection in CharacterSheet, Classes and CharacterClass.
- `AttackDefinition` holds ID/name/routine ID for CharacterSheet and Attacks creation. `SkillSpecializationDefinition` holds ID/name for CharacterSheet and Skills registration, with SkillType passed separately.
- `EnhancementDefinition` is the creation command (effect variant and initial active flag); `EnhancementGroupDefinition` holds group ID/name/initial entries. `EnhancementEffectDefinition` is the closed contribution/modifier variant. EnhancementData/EnhancementGroupData remain distinct saved snapshots; sheet loading explicitly reconstructs creation definitions.
- Strings and collections in these definitions are owned values, not string views or references. Callers may store/reuse definitions or use C++20 designated initializers. The Definition refactor introduced no new persistence fields or rule behavior. The later custom-item catalog uses format 37; see [equipment.md](equipment.md).
- Superseded positional public overloads are removed and repository callers/tests migrated. The C FFI is unchanged. `core/play` is outside this refactor.
