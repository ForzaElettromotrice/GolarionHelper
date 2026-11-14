# Non-human core race coverage

Reviewed against Golarion on 2026-11-13. The six existing race IDs and baseline feature IDs are preserved in `core/sheet/resources/races/core.json`; the human catalog is maintained separately. English references linked from the Italian pages were used for terminology only, never to supply additional rules.

| Race/source | Alternate traits | Favored-class options |
| --- | ---: | ---: |
| [Elf](https://golarion.altervista.org/wiki/Elfo) | 19 | 39 |
| [Gnome](https://golarion.altervista.org/wiki/Gnomo) | 17 | 40 |
| [Halfling](https://golarion.altervista.org/wiki/Halfling) | 20 | 41 |
| [Half-elf](https://golarion.altervista.org/wiki/Mezzelfo) | 14 | 40 |
| [Half-orc](https://golarion.altervista.org/wiki/Mezzorco) | 22 | 39 |
| [Dwarf](https://golarion.altervista.org/wiki/Nano) | 23 | 39 |

The half-orc count includes Squallido, explicitly delegated by its page to the [orc page](https://golarion.altervista.org/wiki/Orco). Multiple alternatives for the same favored class are separate selectable bonuses. All options are class-filtered. Integer benefits use `selectionsPerBenefit` to accumulate fractional selections, while effects with different simultaneous denominators retain explicit manual accounting rather than rounding both at the same threshold.

## Automation and choices

Baseline languages now grant the starting languages and expose race-specific Intelligence lists plus the nonsecret Linguistics catalog. Alternative language profiles replace these registrations. Half-elf Multitalented grants an additional favored-class selection. Magical sleep immunity is registered for elves and half-elves.

Supported numeric benefits use resource modifiers, including conditional attacks, saves, skills, maneuver bonuses/defenses, armor class and movement. Skill specializations use their dotted resource IDs. Benefits that explicitly stack use generic modifiers. Racial modifiers stack in this engine. Woodcraft, Watchful and Greed therefore register only the conditional increment above their permanent bonus; numeric integration assertions verify the resulting totals.

Other supported effects include class skills, natural attacks, resistances, immunities, movement and skill-rank contributions. Discrete choices such as energy type, fixed skills, ability increases, languages or climate are represented as choices where the existing schema can express them. Poliglotta offers a nonsecret language selection independently of Intelligence capacity, including the three languages added during the wider catalog expansion.

## Deliberate manual boundaries

Every reminder, including a partial benefit beside working effects, contains `Ehi, appena hai tempo vedi di correggermi se necessario!`.

- Vision ranges, equipment proficiencies, movement exceptions, rerolls, action economy, social permissions and environmental adjudication remain marked where no applicable endpoint exists.
- Spell-like abilities whose spell IDs are not yet cataloged, caster-level adjustments requiring an actual casting-source ID, class powers, companion/eidolon/familiar statistics and feature-specific daily pools remain explicit marked reminders. They are not represented as unrestricted global bonuses.
- Gnome Gift of Tongues and dwarf Xenophobic change the fixed Linguistics quota. The former retains an explicit extra-language reminder. Xenophobic grants only Dwarven and no ordinary language-option registration, avoiding an incorrectly generous automatic quota; the one-language-per-two-ranks rule is manual.
- Half-elf/half-orc racial equivalence and eligibility to select human/elf/orc options are documented by reminders; unrelated race options are not silently imported.
- Racial subtype descriptions that bundle existing traits are not separate races/presets. Names mentioned only in those descriptions without rules are not filled from English sources.
- Saltbeard changes Greed as well as replacing three traits, so its replacement set includes Greed and restores the modified appraisal benefit explicitly. Deep Jungle similarly replaces the baseline language registration while preserving the ordinary bonus-language list.
- The dwarf cleric favored-class row calls its beneficiary a druid. The option remains cleric-filtered, exposes the discrepancy in a marked reminder, and does not infer a correction from English.

## Validation

The authoring pass checked all replacement references, unique IDs within effect/choice/option arrays, and the mandatory marker on every reminder. Central integration exercises alternate selection/removal, each choice branch, cumulative favored-class thresholds and sheet save/load. Numeric regression assertions cover elf Woodcraft and halfling Watchful/Greed stacking. No separate agent build was run.
