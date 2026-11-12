# Pending Choices

Read when changing completion reporting, choice references or advancement gating.

## Contract

- `CharacterSheet::pendingChoices()` returns detached `PendingChoice` values collected from Identity, Race, Classes (including CharacterClass and FavoredClasses), Feats, Skills, Languages and Spellcastings. Each of these owners exposes its own report method. It does not apply effects, refresh reminders, recheck feats, or mutate saved state.
- A record contains `choiceReference`, an Italian `description`, positive `remaining`, and `blocksAdvancement`. The reference is a closed `std::variant`; `type()` derives the matching enum instead of storing a potentially inconsistent discriminator.
- References carry the IDs needed by existing mutation APIs: racial race/element/choice IDs, class type/acquisition/choice IDs, feat slot ID, class type/level for hit dice and favored bonuses, language origin, casting ID and exact slot/preparation ID, or casting ID/quota ID/spell level for repertoire choices. Sheet-wide race, alignment, favored-class and skill allocation references need no further ID.
- `remaining` counts selections/inputs, not their numerical values: a missing hit die result is one input. Fully completed choices disappear. Reports include only active class acquisitions/racial elements, available empty feat slots, positive allocation quotas and unassigned preparation slots. Excess allocation, an inactive selected feat or a consumed spell slot is not a missing choice.
- The report is derived, not persisted. Pending reports introduce no save-format fields (current sheet format is 41), no additional frontend or C API, and no historical prerequisite reconstruction. `CharacterSheet::addClassLevel` rejects any blocking entry before mutating class state, including at level zero and when advancing a different class. Nonblocking entries do not prevent +1. Removal and saved-state reconstruction remain unrestricted by this gate.

## Agreed blocking policy

| Choice | Blocks advancement |
| --- | --- |
| Missing race or alignment | Yes |
| Unfilled favored-class selection capacity, including additional choices | Yes |
| Available empty source/level feat slot, including restricted slots | Yes |
| Empty user-created extra feat slot | No |
| Active class or racial choice | Its definition's `blocksAdvancement` |
| Missing class-level hit die result | No |
| Unassigned eligible favored-class bonus | No |
| Unspent skill ranks | No |
| Unused Intelligence/Linguistics language choices | No |
| Free prepared slot or unassigned flexible preparation | No |
| Unfilled repertoire quota, including prepared free additions | Yes |

- `ClassChoiceDefinition` and `RacialChoiceDefinition` add `blocksAdvancement = true`. Class/archetype providers set it explicitly when a choice is optional for advancement. Race JSON accepts the optional boolean field with the same default. Corresponding choice Views expose it. Class definitions still have no JSON loader.
- Feat acquisition remains atomic: a feat cannot be selected with incomplete internal choices, so only its empty slot is reported. Item creation likewise requires its internal choices; no incomplete-item state is introduced. Extra feats retain their agreed exemption behavior.
- Spontaneous casting slots need no prepared selection. Source-owned repertoire quotas now report each unfilled quota level via `RepertoireChoiceReference`; see [repertoire.md](repertoire.md). No quotas are inferred from descriptions or class levels. Retained inactive selections are exposed in the repertoire View, not mistaken for empty positions.
- Divinity and other descriptive identity fields are not pending choices. Optional alternate racial traits, archetypes and combat configurations are not treated as mandatory selections.

## Verification

`pending_choices_test` uses a test-only racial catalog and synthetic classes. It covers definition-controlled flags, typed references, partial class selections, nested/replaced/future acquisitions, extra/source slots, favored capacity and bonuses, hit dice, skill/language quotas, prepared/flexible spells, snapshot stability and save/load. `advancement_gate_test` additionally covers the first level, cross-class blocking, class/racial/source-feat choices, favored capacity, repertoire quotas in all three casting modes (including reopening and manual-addition independence), nonblocking omissions, unchanged saves on rejection and unrestricted -1/load. Only sheet tests are run.
