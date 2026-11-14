# Class Feature Option Catalogs

Read when changing options of the 44 playable classes. Source review now covers all 44; supported finite selections are attached, while explicitly manual/delegated families remain documented rather than being misrepresented as automated. Rules and discovery use Golarion only. Existing class/runtime boundaries are in [classes.md](classes.md) and [class-catalog.md](class-catalog.md).

## Runtime Input Contract

Files under `core/sheet/resources/class-options/` contain objects with exactly `optionSets` and `bindings` arrays. Source/provenance reports are separate `*.sources.json` maintenance files, never runtime inputs.

```json
{
  "optionSets": [{
    "id": "barbarian.ragePowers", "name": "Poteri d'ira", "description": "Fonte: ...",
    "options": [{
      "id": "powerStableId", "name": "Nome italiano", "description": "Prerequisiti e fonte; limiti manuali con marker.",
      "progression": [{"id": "initial", "level": 1, "effects": [{"id": "manual", "type": "reminder", "message": "Applicazione manuale ... Ehi, appena hai tempo vedi di correggermi se necessario!"}]}]
    }]
  }],
  "bindings": [{
    "id": "barbarian.ragePowerChoices", "classType": "barbarian", "featureId": "existing.feature.id",
    "acquisitionIds": ["existing.acquisition.id"],
    "choice": {"id": "ragePower", "prompt": "Scegli un potere d'ira", "selectionCount": 1, "blocksAdvancement": true, "optionSet": "barbarian.ragePowers"}
  }]
}
```

- `classDefinitionsWithOptionsFromJson` resolves the sidecar catalogs and returns ordinary `ClassDefinition` values. No new runtime manager or save format is needed; existing choice APIs/Data/View are reused. Typed custom managers remain unchanged.
- Set IDs, option IDs, binding IDs and choice IDs are stable English catalog identifiers (hashes are acceptable), not open rule categories. Class types/effects are parsed through existing closed enums.
- A choice can use either `optionSet` or inline `options`, never both. Nested choices in option progressions can reference another set. Cyclic/unknown references reject. Unknown catalog fields reject rather than silently ignoring new semantics.
- An option progression's IDs are local to that option. The loader prefixes them with the owning acquisition/choice/option path, so repeated selections do not collide globally. `replaces` may reference only that option's own direct progression stages; original-class or cross-option structural changes are not supported here. Nested options have their own namespaces.
- Target feature and acquisition IDs must already exist in the owning base class. Do not change original IDs: existing archetype structural edits depend on them. Report missing genuine original privileges to the main agent before creating bindings.
- A binding can set `retainEarlierAcquisitions: true` only when the source really grants cumulative recurring selections. This removes replacement links between that binding's original acquisition targets, preserving earlier selections instead of treating them as numeric upgrades. The loader permits this only for original acquisitions with no automatic effects or existing choices, preventing duplicated mechanical bonuses. Do not use it for ordinary rank/scaling upgrades.
- Scope is base-class options, including the four unchained versions. Archetypes inherit unaffected choices; new archetype-specific option sets/conditional eligibility are not automatically attached by name matching. Do not globally mix archetype-only options into a normal class list.

## Source and Modeling Policy

- Discover each class's real choice families from its Golarion page, linked rules pages and setting-class pages. Do not treat every heading as a selectable option: tables, containers, automatic powers, examples and feat catalogs can be different systems.
- Attach mandatory permanent choices with `blocksAdvancement: true`; daily, optional, swap/recovery choices do not block advancement. A whole family selected once must not be attached to every later automatic upgrade. Recurring acquisitions get distinct owning acquisition IDs, using the actual source cadence.
- Preserve existing caster IDs, slot/repertoire groups and original static effects. Restricted domain/school/spirit spells must not become unrestricted slots or fake known spells; class-local spell levels and eligibility are existing documented gaps. Add only supported reversible effects with source-correct scope.
- Current `ClassChoiceDefinition` does not enforce option prerequisites, once-only use across recurring acquisitions, domain/subdomain exclusions, conditional pools or simultaneous/family limitations. These remain explicitly manual, with source text summarized in option descriptions/reminders. Do not claim validation. For repeated restricted families, keep potentially duplicating benefits manual rather than automatically stacking a once-only benefit from repeated invalid selections.
- Ability-replacement effects expose alternatives, not forced changes to generic totals. Active powers, timing, item/companion statistics and spell-list extensions can remain manual. Use the exact marker on every manual reminder: `Ehi, appena hai tempo vedi di correggermi se necessario!`.
- Expressions accept numbers, `@targets` and arithmetic/comparison operators, not `max`, `min` or `floor` functions. Modifiers cannot resolve to negative values; signed corrections belong in actual contribution endpoints where semantically appropriate.
- Do not duplicate actual feat selection into class choices when the existing Feats subsystem already owns that slot. Record delegated families explicitly. Do not introduce spellcasting/creature models or broad backend refactors to conceal data limitations.
- Source pages that are missing, ambiguous or cannot be bound without incorrect automatic effects remain explicit pending entries. Never import rules from another site or an unverified memory to fill them.

## Work Layout

### Current Coverage

The integrated catalog contains 85 reusable sets, 2,617 option templates and 91 bindings. Templates include deliberate base/advanced pool unions and parent-specific variants; this is not a count of distinct Pathfinder abilities. All 44 playable classes were reviewed; 42 have attached choices. Ordinary Monk and Swashbuckler do not acquire an independent permanent option family here: their feat choices belong to Feats and automatic/use-time abilities are not fabricated selections.

| Runtime file | Sets | Templates | Bindings | Main families |
| --- | ---: | ---: | ---: | --- |
| `barbarian.json` | 1 | 156 | 1 | Rage powers |
| `wizard.json` | 4 | 81 | 2 | Schools, opposed schools, bonds, familiar species and bonded-item categories |
| `divine.json` | 3 | 140 | 2 | Domains/subdomains and druid nature bond |
| `core-remaining.json` | 12 | 525 | 12 | Versatile performance, weapon groups/mastery, rogue talents, mercies, ranger enemies/terrain/styles/bond and sorcerer bloodlines |
| `advanced.json` | 21 | 633 | 23 | Discoveries, orders, eidolon forms, hexes/patrons/familiars, domains/inquisitions, arcana, aspects, mysteries/curses/revelations, firearms/deeds and vigilante talents |
| `alternative.json` | 6 | 113 | 6 | Cruelties/bond, ninja tricks, samurai orders/weapon expertise |
| `hybrid.json` | 15 | 331 | 18 | Exploits, maneuvers, animal focus, talents, bloodlines, blessings, rage powers, spirits/hexes/familiar; nested dragon/element choices |
| `occult.json` | 15 | 330 | 19 | Elements/blasts/infusions/utility talents, daily spirits, tricks/stares, implement schools/focus powers, disciplines/amplifications and emotional foci |
| `unchained.json` | 8 | 308 | 8 | Rage powers, eidolon subtypes/forms, rogue talents/skill unlocks, ki powers/style strikes |

All new option benefits are manual reminders, not newly automated mechanical contributions. Selection, nested pending choices and persistence use the ordinary class model. Each runtime file has a separate source report with explicit pending/delegated families. In particular, variable-point evolutions, companion creature state, conditional sublists/secondary choices, feat/repertoire alternatives and restricted spell grants are not silently automated or granted twice.

Existing source ownership is preserved. A dedicated `sorcerer.bloodlineSelection.level1` acquisition was added from the actual level-1 bloodline rule, so replacing a first-level bloodline power does not incorrectly remove the entire bloodline choice. Original power IDs remain unchanged. Shifter's fifth aspect belongs to its original level-20 final-aspect acquisition. Unchained Monk learns style strikes at 5/9/13/17, not at 15 (which increases uses per round). Skald's original table owner at level 16 is retained; its prose instead implies 17, and this source discrepancy is marked in the report/reminder.

Verification on 2026-11-13: full `core/sheet` build succeeds and all 102 sheet tests pass (103.72 seconds). Expanded `class_catalog_test` fills active choices for every class at level 20, checks nested pending completion, save/load, descent to zero with complete endpoint cleanup and reactivation with preserved choice IDs. Existing archetype lifecycle, affected-choice reset and source-casting continuity checks also pass. The strict loader test covers malformed fields/references/cycles and acquisition namespacing. The data audit confirms all 85 sets are reachable, 2,617 marked templates, 91 unique bindings spanning 311 acquisition/choice targets, only Golarion source URLs, 44 reviewed classes and 762 base feature records. `git diff --check` passes. Version `1.1.0` and save format `42` are unchanged; `core/play` tests were not run. Publication of the accumulated catalogs, code, tests and context was requested on 2026-11-14. Local Gradle/build artifacts and the CodexSync workspace identifier are excluded from Git publication.

The main agent owns the loader, integration, shared class-data corrections, tests and final documentation. Worker runtime/report files must have disjoint names and class ownership. Preserve all pre-existing uncommitted race/spell/feat/class/archetype work; do not commit or push without a request.
