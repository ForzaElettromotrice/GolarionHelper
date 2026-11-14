# Class Archetype Catalog

Read before changing archetype data or reviewing missing automation. Runtime ownership and compatibility are in [archetypes.md](archetypes.md); owning class definitions are in [class-catalog.md](class-catalog.md).

## Coverage and Sources

- Scope: class archetypes for the 44 playable classes, using only Golarion. Prestige classes, NPC classes and creature templates are excluded.
- The source inventory contains **764 reviewed class/source entries**: **661 included runtime definitions** and **103 pending entries**. The latter are not selectable through the default manager. This count includes separately selectable oaths, ordinary/unchained variants, and a few nonselectable source containers or misplaced links; it is not a claim that every inventory row is a distinct playable archetype.
- Discovery includes the class introduction tables, linked archetype pages, nested sections such as the eleven Animal Shamans, and the setting-class pages linked by Golarion's class-options index. It finds Samurai's Santo della Spada even though the Samurai introduction has no archetype table.
- `core/sheet/resources/archetypes/catalog.sources.json` is the consolidated provenance/status index; the three `*.sources.json` group reports preserve individual review notes. They are maintenance data, not runtime input.

| Runtime file | Source entries | Included | Pending |
| --- | ---: | ---: | ---: |
| `core-early.json` | 282 | 249 | 33 |
| `core-advanced.json` | 262 | 226 | 36 |
| `other.json` | 220 | 186 | 34 |

Source IDs remain distinct for genuinely different profiles with the same Italian name (for example the two Fighter Guardiano del Sapere sources). The first 22 reviewed Barbarian/Unchained Barbarian IDs are preserved in the expanded files; their temporary seed file is removed.

## Applied Rules

- Complete structural impacts are declared against real original feature IDs. Whole losses disable original effects at every level; partial losses use precise acquisition IDs where the original representation supports them. Unaffected registrations remain intact. Removing an archetype restores the original class.
- Class-skill additions use existing reversible `classSkillGrant` effects and the metadata-only `<classType>.classSkills` compatibility identity. Other metadata identities exist only where concrete archetypes need them: Cleric/Druid BAB and rank rate, Fighter rank rate, Kineticist/Vigilante base saves, and Wizard spellbook.
- BAB, base-save and freely allocatable rank-rate changes use exact difference contributions through existing endpoints. Rank corrections preserve the per-level minimum and live `@intSkillRanksMod` changes; expressions use arithmetic/comparisons, not unsupported `max()` functions. Restricted skill-point awards do not become unrestricted rank budgets.
- Selected supported defenses, DR, movement, skills, languages and other numeric benefits are applied through existing effects. New contextual powers, option catalogs, timing, racial/alignment eligibility and other unsupported portions remain source-linked Italian reminders ending with `Ehi, appena hai tempo vedi di correggermi se necessario!`. Presence is not full automation.
- Mandatory ability substitutions can expose the existing alternative in the View, but the generic core target is not forcibly changed. Cultista dei Miti Antichi and Monaco del Loto mark this explicitly; the player must use the required alternative. Do not label an optional-alternative endpoint as a forced replacement.
- Both [Barbarian Unchained](https://golarion.altervista.org/wiki/Barbaro_Rivisitato) and [Rogue Unchained](https://golarion.altervista.org/wiki/Ladro_Rivisitato) explicitly permit Danger Sense to replace Trap Sense for archetype substitutions. The corresponding report records this source-backed adaptation; it is not a guessed synonym.
- Source terminology corrections stay local and documented. Paladin Aura di Risolutezza is mapped to Aura di Fermezza by an explicitly documented contextual inference (the unique level-eight aura in the same class). Actual contradictory concepts are not silently corrected from English sources.

## Casting Continuity

- Changes to components, focus, key ability or other properties already manual in the base provider retain the existing casting instance, slots and repertoire where those remain valid.
- Slot reductions and cantrip-slot losses replace only the original slot acquisitions. New source-owned groups reference the same original casting ID; owner and repertoire quotas/selections remain alive. External grants keep their target. Removing the archetype restores base groups; changed slot groups require preparation/use reassignment as usual.
- Losing level-zero slots is not a complete ban on higher-slot cantrip preparation in the generic runtime; the source restriction remains explicitly manual.
- Sanguemisto reduces known-spell quotas; Mesmerista Ombratile reduces daily slots. Incanalatore Nexiano removes only level-zero quota access while retaining positive capacities in its two mixed groups; positive selections attached to those replaced quota IDs need reassignment, while withdrawn selections remain retained by the repertoire system.
- List/mode changes that require coordinated caster replacement, new class-local spell levels or a different repertoire state remain pending rather than keeping forbidden base choices or cloning a caster with broken external bindings. Examples include Arcanista Illetterato, Richiamatore Selvaggio, several prepared/spontaneous conversions, and Parapsichico Amnesico.
- Legitimate sources that remove spellcasting entirely do revoke its owner; providers depending on that owner's explicit ID must obey the existing consumer-before-owner lifetime contract. This dataset step does not add automatic cross-source retargeting.

## Pending Boundaries

Each pending inventory row records its own source and reason. Main groups:

- Withdrawal/replacement of class-owned skill grants is unsupported; simply adding the replacement skill would leave a false original class skill.
- Qinggong/classic Aspide and the optional companion choice of Marine della Compagnia Nera need choices that determine structural impacts/removals and compatibility. Unchained Aspide has a different source and remains included; its extra powers are ordinary Ki options.
- Ouat replaces automatic Dwarf racial traits, outside class-only structural edits.
- Source contradictions or missing anchors are preserved, including Crociato Affranto, Fauce Ferale's ambiguous Veloce, mismatched feature names/levels, and two Druid sources naming extra Wild Empathy uses the base class does not possess.
- Paladino del Giuramento is a container, not one archetype applying every oath's losses. Its individual oaths are separate included definitions.

## Loading, Persistence and Verification

- `archetypeDefinitionFromJson` / `archetypeDefinitionToJson` reuse the strict nested feature parser. Class identity and Alter/Replace are closed enums. Blank/duplicate IDs, malformed collections, unknown effects, undeclared removals, incomplete whole replacements and local replacement cycles reject.
- Cross-references against the concrete class remain validated by `CharacterClass::addArchetype`. Default loading embeds only the three runtime arrays; typed custom managers remain supported.
- No save-format or library-version change. Existing class Data/View store archetype IDs and choices; no play-module changes or tests.
- The integrated catalog test exercises every included archetype through 0–20–0, Data reconstruction, removal/reinsertion, original-state restoration and cleanup. It also evaluates derived Views with ability scores 8/18 and has source-specific golden checks for BAB/rank minima, base saves, DR, compatibility and casting/repertoire/external-grant continuity.
- Verification on 2026-11-13: the complete sheet build succeeds and all 101 sheet tests pass. Independent worker probes also passed for all three groups. No play-module tests were run.
- All source rows are reviewed, but pending entries and marked mechanics still require follow-up. Temporary extraction helpers under `/tmp` are research aids, not authoritative project material.

## Next Steps

Implement one explicit missing behavior at a time, starting with source-specific class-skill withdrawal if requested, then revisit the affected pending rows. Conditional structural choices, list/mode/state changes and cross-racial replacements are separate backend slices. Do not infer runtime rules from Italian names or bulk-enable pending profiles merely because their JSON parses.
