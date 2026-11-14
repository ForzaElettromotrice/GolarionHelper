# Race catalog expansion

## Scope and status

User requested all playable races, not only core races. Scope: all 63 entries in [Golarion's race index](https://golarion.altervista.org/wiki/Razze), checked 2026-11-13. Process each race's standard traits, languages, alternate traits/replacements and favored-class options. Follow linked English references for identifiers. Race-specific items, spells, feats and class archetypes belong to their own deferred datasets; do not silently grant them.

Preserve supported effects and source cleanup. All manual benefits, including partial ones, must carry `Ehi, appena hai tempo vedi di correggermi se necessario!`. A reminder is not an automated benefit. Unknown rules remain explicitly unresolved rather than inferred from similar races. Do not mark a race complete merely because its base profile exists.

## Checklist

All 63 entries are reviewed. This means page coverage, not complete automation. The catalog contains 278 alternate entries and 485 class-filtered favored options. No English-only rules were imported.

Data is split between `resources/races.json` (Human) and `resources/races/{core,uncommon,rare,other}.json`, embedded together by CMake. Coverage and manual boundaries: [Human](human-catalog.md), [other core races](race-core-coverage.md), [uncommon](race-uncommon-coverage.md), [unusual](race-rare-coverage.md), [other races](race-other-coverage.md).

`race_catalog_test` checks every baseline, alternate and choice, favored rewards through their thresholds, save/load, cleanup and mandatory reminder markers. Independent numeric assertions cover selected movement, defense and conditional-stacking examples.

Final verification: sheet-only build and all 96 sheet tests pass (2026-11-13), including the complete catalog integration test. `git diff --check` is clean. No play tests were run; no commit or push was requested for this expansion.

| Group | Race/source | Status |
| --- | --- | --- |
| Base | [Elfo](https://golarion.altervista.org/wiki/Elfo) | reviewed |
| Base | [Gnomo](https://golarion.altervista.org/wiki/Gnomo) | reviewed |
| Base | [Halfling](https://golarion.altervista.org/wiki/Halfling) | reviewed |
| Base | [Mezzelfo](https://golarion.altervista.org/wiki/Mezzelfo) | reviewed |
| Base | [Mezzorco](https://golarion.altervista.org/wiki/Mezzorco) | reviewed |
| Base | [Nano](https://golarion.altervista.org/wiki/Nano) | reviewed |
| Base | [Umano](https://golarion.altervista.org/wiki/Umano) | reviewed; see human-catalog.md |
| Peculiari | [Aasimar](https://golarion.altervista.org/wiki/Razze/Aasimar) | reviewed |
| Peculiari | [Coboldo](https://golarion.altervista.org/wiki/Razze/Coboldo) | reviewed |
| Peculiari | [Dhampir](https://golarion.altervista.org/wiki/Razze/Dhampir) | reviewed |
| Peculiari | [Drow](https://golarion.altervista.org/wiki/Razze/Drow) | reviewed |
| Peculiari | [Felinide](https://golarion.altervista.org/wiki/Razze/Felinide) | reviewed |
| Peculiari | [Ghermito](https://golarion.altervista.org/wiki/Razze/Ghermito) | reviewed |
| Peculiari | [Goblin](https://golarion.altervista.org/wiki/Razze/Goblin) | reviewed |
| Peculiari | [Hobgoblin](https://golarion.altervista.org/wiki/Razze/Hobgoblin) | reviewed |
| Peculiari | [Ifrit](https://golarion.altervista.org/wiki/Razze/Ifrit) | reviewed |
| Peculiari | [Ondine](https://golarion.altervista.org/wiki/Razze/Ondine) | reviewed |
| Peculiari | [Orco](https://golarion.altervista.org/wiki/Razze/Orco) | reviewed |
| Peculiari | [Oreade](https://golarion.altervista.org/wiki/Razze/Oreade) | reviewed |
| Peculiari | [Rattoide](https://golarion.altervista.org/wiki/Razze/Rattoide) | reviewed |
| Peculiari | [Silfide](https://golarion.altervista.org/wiki/Razze/Silfide) | reviewed |
| Peculiari | [Tengu](https://golarion.altervista.org/wiki/Razze/Tengu) | reviewed |
| Peculiari | [Tiefling](https://golarion.altervista.org/wiki/Razze/Tiefling) | reviewed |
| Insolite | [Changeling](https://golarion.altervista.org/wiki/Razze/Changeling) | reviewed |
| Insolite | [Drow Nobile](https://golarion.altervista.org/wiki/Razze/Drow_Nobile) | reviewed |
| Insolite | [Duergar](https://golarion.altervista.org/wiki/Razze/Duergar) | reviewed |
| Insolite | [Grippli](https://golarion.altervista.org/wiki/Razze/Grippli) | reviewed |
| Insolite | [Kitsune](https://golarion.altervista.org/wiki/Razze/Kitsune) | reviewed |
| Insolite | [Marinide](https://golarion.altervista.org/wiki/Razze/Marinide) | reviewed |
| Insolite | [Nagaji](https://golarion.altervista.org/wiki/Razze/Nagaji) | reviewed |
| Insolite | [Samsaran](https://golarion.altervista.org/wiki/Razze/Samsaran) | reviewed |
| Insolite | [Strix](https://golarion.altervista.org/wiki/Razze/Strix) | reviewed |
| Insolite | [Suli](https://golarion.altervista.org/wiki/Razze/Suli) | reviewed |
| Insolite | [Svirfneblin](https://golarion.altervista.org/wiki/Razze/Svirfneblin) | reviewed |
| Insolite | [Uomini Pesce](https://golarion.altervista.org/wiki/Razze/Uomini_Pesce) | reviewed |
| Insolite | [Vanara](https://golarion.altervista.org/wiki/Razze/Vanara) | reviewed |
| Insolite | [Vishkanya](https://golarion.altervista.org/wiki/Razze/Vishkanya) | reviewed |
| Insolite | [Wayang](https://golarion.altervista.org/wiki/Razze/Wayang) | reviewed |
| Altre | [Androide](https://golarion.altervista.org/wiki/Razze/Androide) | reviewed |
| Altre | [Astomoi](https://golarion.altervista.org/wiki/Razze/Astomoi) | reviewed |
| Altre | [Caligni](https://golarion.altervista.org/wiki/Razze/Caligni) | reviewed |
| Altre | [Centauro](https://golarion.altervista.org/wiki/Razze/Centauro) | reviewed |
| Altre | [Drider](https://golarion.altervista.org/wiki/Razze/Drider) | reviewed |
| Altre | [Gargoyle](https://golarion.altervista.org/wiki/Razze/Gargoyle) | reviewed |
| Altre | [Gathlain](https://golarion.altervista.org/wiki/Razze/Gathlain) | reviewed |
| Altre | [Ghoran](https://golarion.altervista.org/wiki/Razze/Ghoran) | reviewed |
| Altre | [Gnoll](https://golarion.altervista.org/wiki/Razze/Gnoll) | reviewed |
| Altre | [Goblin Scimmia](https://golarion.altervista.org/wiki/Razze/Goblin_Scimmia) | reviewed |
| Altre | [Ibrido Abitatore del Profondo](https://golarion.altervista.org/wiki/Razze/Ibrido_Abitatore_del_Profondo) | reviewed |
| Altre | [Kasatha](https://golarion.altervista.org/wiki/Razze/Kasatha) | reviewed |
| Altre | [Lucertoloide](https://golarion.altervista.org/wiki/Razze/Lucertoloide) | reviewed |
| Altre | [Munavri](https://golarion.altervista.org/wiki/Razze/Munavri) | reviewed |
| Altre | [Naiade](https://golarion.altervista.org/wiki/Razze/Naiade) | reviewed |
| Altre | [Ogre](https://golarion.altervista.org/wiki/Razze/Ogre) | reviewed |
| Altre | [Orang-Pendak](https://golarion.altervista.org/wiki/Razze/Orang-Pendak) | reviewed |
| Altre | [Rettiloide](https://golarion.altervista.org/wiki/Razze/Rettiloide) | reviewed |
| Altre | [Rougarou](https://golarion.altervista.org/wiki/Razze/Rougarou) | reviewed |
| Altre | [Shabti](https://golarion.altervista.org/wiki/Razze/Shabti) | reviewed |
| Altre | [Trox](https://golarion.altervista.org/wiki/Razze/Trox) | reviewed |
| Altre | [Vivarano](https://golarion.altervista.org/wiki/Razze/Vivarano) | reviewed |
| Altre | [Wyrwood](https://golarion.altervista.org/wiki/Razze/Wyrwood) | reviewed |
| Altre | [Yaddithian](https://golarion.altervista.org/wiki/Razze/Yaddithian) | reviewed |
| Altre | [Yenaldooshi](https://golarion.altervista.org/wiki/Razze/Yenaldooshi) | reviewed |

## Source discrepancies

- Android: Golarion's narrative and racial table differ about biological immunities. The profile exposes this ambiguity as a marked reminder rather than silently treating humanoids as full constructs. Morale-bonus suppression and Nanite Surge remain manual. The Golarion page lists no alternate traits or favored-class bonuses; no English-only additions are imported.
