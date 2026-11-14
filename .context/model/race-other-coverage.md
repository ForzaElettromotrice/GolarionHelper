# Other race coverage

Read when reviewing the 25 entries in `resources/races/other.json`. Reviewed 2026-11-13 against the corresponding pages under [Golarion's Other Races](https://golarion.altervista.org/wiki/Razze). Individual sources are linked in [the catalog checklist](race-catalog.md). Linked English references supplied terminology only.

## Scope

All 25 pages have a base profile represented. None of these Golarion pages provides a selectable alternate-trait or favored-class-bonus list; those arrays are intentionally empty. English-only alternatives, heritage tables and favored bonuses are not imported. In particular, Skinwalker mentions heritages without listing their rules; it does not imply importing the English heritage table.

The profiles use the race-building/player entries, not the Bestiary monsters' racial Hit Dice, ability scores or attacks. CR notes do not silently grant class levels.

## Automated mechanics

- Listed ability modifiers, sizes, carrying body type and native movement (including flying maneuverability).
- Initial languages, explicit Intelligence lists and nonsecret Linguistics options. Missing Intelligence lists remain unavailable with a marked reminder, rather than inventing permission.
- Skill and save modifiers, natural armor, dodge defenses, class-skill grants, maneuver defenses, energy/spell resistance and damage reduction.
- Explicit informational immunities. Conditional numerical effects remain caller-selected contexts, as elsewhere in the backend.
- Static natural attacks for Gargoyle, Lizardfolk, Rougarou and Yaddithian. Gargoyle/Lizardfolk claws and gore follow Golarion's [natural-attack table](https://golarion.altervista.org/wiki/Attacchi_Naturali); the bite follows the reduced Medium 1d3 damage of the [linked racial Bite trait](https://golarion.altervista.org/wiki/Razze/Tratti#Morso_(1_PR)), not the Bestiary profile. Hand occupancy and mixed-routine selection remain explicitly marked manual.

## Manual boundaries

Every manual remainder is a reminder with the agreed marker. These include senses and communication, creature-type interactions, spell-like abilities missing from the spell catalog, shapeshifting, durations/daily activation, companion/recipient effects, terrain exceptions, physiology and extra appendages. Reptoid attacks depend on a runtime form that the model does not own; Wyvaran's tail has an opportunity-only restriction. They remain marked instead of exposing unrestricted always-active attacks.

Wyrwood is especially incomplete mechanically: absent Constitution is not representable by a numeric score, and setting it to zero would be wrong. Its visible reminder explicitly warns that Constitution-dependent calculations, HP and Fortitude need manual correction. Construct traits are not silently inherited as though they were humanoid traits.

Android follows Golarion's racial table. The contradictory biological prose is flagged; immunities found only in the English rules were not added. Ghoran explicitly does NOT receive the usual plant immunities. Astomoi's type is unspecified on its racial page and is not guessed.

Drider uses quadruped carrying rules for its multi-legged body, with octoped +12 trip defense and a marked anatomical limitation. Ogre's 3 m reach must currently be assigned to individual attacks; no fake global reach resource is registered.

## Languages

The racial pages add the nonsecret IDs `reptoid`, `rougarou`, `yaddithian`. Caligni explicitly starts with `darkFolk`; Munavri explicitly permits it as an Intelligence option. Neither exception makes it globally learnable. Yaddithian's unrestricted wording does not clearly address secret languages; the automated list remains nonsecret and this limit is marked. Naiad's regional human languages reuse the lists already reviewed for Human.

## Verification

`race_catalog_test` loads each definition, applies/clears baseline effects, exercises racial choices and replacements, and round-trips saved configurations. Numeric examples check Centaur body/size/speed, Drider climbing/RI, Gargoyle flight/RD, Ghoran class skills/no inherited immunities, Android penalties/immunities and Yaddithian resistance cleanup. No play tests are run.
