# Uncommon race catalog coverage

Read when changing the sixteen `Razze Peculiari` in `core/sheet/resources/races/uncommon.json`. Reviewed against Golarion on 2026-11-13. All rule content comes from the Italian pages; linked English references were consulted only for identifiers/terminology, never to add missing rules or options.

## Page coverage

The file contains 16 base profiles, all 101 alternate traits listed in these pages, and 138 favored-class options covering 137 source bullets (the Tiefling sorcerer's two alternatives are separate choices). Every favored option has the specific class enum filter; no option automatically makes that class favored. Positive integer selection groups represent fractional awards. Defaults remain the normal HP/skill-rank alternatives.

| Race ID | Golarion source | Alternate traits | Favored options |
| --- | --- | --- | --- |
| `aasimar` | [Aasimar](https://golarion.altervista.org/wiki/Razze/Aasimar) | 9 | 8 |
| `kobold` | [Coboldo](https://golarion.altervista.org/wiki/Razze/Coboldo) | 4 | 15 |
| `dhampir` | [Dhampir](https://golarion.altervista.org/wiki/Razze/Dhampir) | 3 | 7 |
| `drow` | [Drow](https://golarion.altervista.org/wiki/Razze/Drow) | 6 | 7 |
| `catfolk` | [Felinide](https://golarion.altervista.org/wiki/Razze/Felinide) | 6 | 7 |
| `fetchling` | [Ghermito](https://golarion.altervista.org/wiki/Razze/Ghermito) | 5 | 6 |
| `goblin` | [Goblin](https://golarion.altervista.org/wiki/Razze/Goblin) | 7 | 12 |
| `hobgoblin` | [Hobgoblin](https://golarion.altervista.org/wiki/Razze/Hobgoblin) | 9 | 9 |
| `ifrit` | [Ifrit](https://golarion.altervista.org/wiki/Razze/Ifrit) | 8 | 8 |
| `undine` | [Ondine](https://golarion.altervista.org/wiki/Razze/Ondine) | 9 | 7 |
| `orc` | [Orco](https://golarion.altervista.org/wiki/Razze/Orco) | 4 | 7 |
| `oread` | [Oreade](https://golarion.altervista.org/wiki/Razze/Oreade) | 8 | 8 |
| `ratfolk` | [Rattoide](https://golarion.altervista.org/wiki/Razze/Rattoide) | 4 | 10 |
| `sylph` | [Silfide](https://golarion.altervista.org/wiki/Razze/Silfide) | 8 | 8 |
| `tengu` | [Tengu](https://golarion.altervista.org/wiki/Razze/Tengu) | 4 | 7 |
| `tiefling` | [Tiefling](https://golarion.altervista.org/wiki/Razze/Tiefling) | 7 | 12 |

## Automated and manual boundaries

- Base ability modifiers, size, carrying body type, movement grants, ordinary languages/learning lists, supported skill/save/attack/AC/CMB/CMD bonuses, energy resistance, spell resistance and informational immunities use existing effects. Natural attacks are actual strike grants, including paired claws and choice-based Tiefling bite/claws. Size-derived modifiers are not duplicated.
- Situational numeric modifiers use explicit caller-selected Italian contexts. The dataset does not infer world position, enemy type, current class feature or activation from prose.
- Every unsupported portion has its own reminder ending exactly `Ehi, appena hai tempo vedi di correggermi se necessario!`. This includes partial traits, class-restricted casting/quotas, companions, daily activation/cooldowns, special senses, weapon proficiency, open skill specializations, class-level requirements not exposed as effects and modifier caps. A reminder does not grant the described benefit.
- No missing spells/feats are referenced as actual grants; their names, limits and eligibility remain in marked reminders until those definitions/providers exist. No unrestricted feat slot substitutes for a specific feat.
- Fractional favored rewards trigger after the declared number of selections. Manual rewards describe a completed unit, except explicitly nonlinear rewards (Aasimar eidolon DR, Goblin alchemist resistance) and the mixed-rate Undine monk reward. The latter automatically adds one grapple CMD per selection and separately reminds the player to accumulate one-third Stunning Fist use; its usage fraction is manual, not applied every selection.
- Open Craft/Profession specializations are not invented as fixed IDs and no broad all-Craft bonus is used as a substitute. Kobold Perform is a whole-category bonus/class-skill grant because the source explicitly applies it to Perform rather than one specialization.
- Resistance accumulation for Goblin alchemist remains manual: multiple independent resistance grants use maximum, not sum; using repeated fixed-value grants would silently cap the benefit at one.

## Important source-specific details

- Aasimar Celestial language is a separate baseline feature so Scion of Humanity removes only that language, not Common or learning options. Exalted Resistance has an explicit evil-spell/evil-outsider applicability. Deathless Spirit's negative energy resistance and save bonus are real effects, but the no-HP-loss exception to negative levels is manual.
- Dhampir Dayborn removes both its spell-like grant and light sensitivity. Fangs is deliberately not a free ordinary bite: its Golarion restriction to damaging in a grapple or biting helpless/bound/paralyzed targets remains manual.
- Drow explicitly permits `drowSignLanguage` as an Intelligence language option; this exception is not added to the global nonsecret list. Seducer uses the Italian **Wisdom 15** requirement, not the different English requirement. Information about this eligibility remains in the marked reminder.
- Catfolk Curiosity grants History/Local as class skills, but the alternative +2 benefit when the class already supplies them remains manual. Climber grants climb speed and the associated +8 skill bonus. Nimble Faller automates trip CMD, not physical falling resolution.
- Fetchling Disguise Self, level-9 Shadow Walk and level-13 Plane Shift are distinct baseline elements so Subtle Manipulator and Gloom Shimmer can coexist without falsely replacing each other. World Walker preserves Stealth. The restricted D'ziriak comprehension is a marked reminder, not a spoken language grant: the current language model cannot represent comprehension-only selections. Regional human Intelligence language IDs reuse catalog IDs; ordinary Linguistics has the broad nonsecret list.
- Hobgoblin Bandy-Legged replaces the base movement grant, retains Sneaky, and adds its skill/ground-dependent defense bonuses. The Italian page does not say it replaces Sneaky. Scarred removes Darkvision as declared.
- Orc Feral replaces weapon familiarity and a separate automatic-language feature; it keeps the Intelligence/Linguistics option providers, allowing learned languages while removing Common/Orc source grants.
- The elemental races' apparent Charisma increases affect only named bloodlines; they are reminders rather than unconditional Charisma modifiers. Triggered healing never becomes always-on regeneration. Sylph Breeze-Kissed exposes its conditional AC but leaves activation, daily maneuver and 24-hour suppression manual.
- Tiefling chooses Abyssal or Infernal as a racial choice, not both. Soul Seer has no daily-use number in the Italian page; its reminder explicitly asks for GM handling rather than importing the English at-will rule. Sorcerer bonus alternatives retain their prerequisite power requirement as marked manual text.
- Race-based items, feats, spells, archetypes, rogue talents and discoveries linked outside the racial traits/favored bonuses are separate datasets, not automatically granted by selecting the race. English-only heritage tables/options are not imported.

## Validation handoff

Source-count audit confirmed 101/101 alternate entries and 137/137 favored source bullets. JSON-level checks confirm all alternate replacements refer to baseline IDs and every reminder carries the marker. A temporary C++ validator linked to the existing sheet libraries successfully parsed all sixteen definitions. A second validator applied each baseline/alternate combination, each alternate choice option and every favored reward's effects through EnhancementGroup on CharacterSheet, evaluated Views, and removed the groups; all sixteen races passed. This tests endpoint/schema compatibility but not Race-owned persistence or favored-class selection gates. Central `race_catalog_test` also covers Race-owned persistence, favored-class thresholds and cleanup across the complete embedded catalog.
