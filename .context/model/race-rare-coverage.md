# Unusual race catalog coverage

## Scope

Reviewed 2026-11-13 against the 15 `Razze Insolite` pages linked by [Golarion's race index](https://golarion.altervista.org/wiki/Razze). Data is in `core/sheet/resources/races/rare.json`. Includes standard profiles, language grants/options, every alternate-trait entry and every favored-class option on those pages. Descriptions paraphrase rules; English references supply terminology only. English-only additions are intentionally absent.

Every manual remainder carries exactly `Ehi, appena hai tempo vedi di correggermi se necessario!`, including partially automated traits and benefits. Presence in the catalog is not full mechanical automation.

## Coverage

| ID | Golarion page | Alternate entries | Favored options | Favored options with manual remainder |
| --- | --- | ---: | ---: | ---: |
| changeling | [Changeling](https://golarion.altervista.org/wiki/Razze/Changeling) | 3 | 3 | 3 |
| drowNoble | [Drow Nobile](https://golarion.altervista.org/wiki/Razze/Drow_Nobile) | 6 | 7 | 6 |
| duergar | [Duergar](https://golarion.altervista.org/wiki/Razze/Duergar) | 5 | 3 | 1 |
| grippli | [Grippli](https://golarion.altervista.org/wiki/Razze/Grippli) | 4 | 5 | 3 |
| kitsune | [Kitsune](https://golarion.altervista.org/wiki/Razze/Kitsune) | 2 | 5 | 3 |
| merfolk | [Marinide](https://golarion.altervista.org/wiki/Razze/Marinide) | 3 | 3 | 3 |
| nagaji | [Nagaji](https://golarion.altervista.org/wiki/Razze/Nagaji) | 1 | 4 | 3 |
| samsaran | [Samsaran](https://golarion.altervista.org/wiki/Razze/Samsaran) | 1 | 4 | 3 |
| strix | [Strix](https://golarion.altervista.org/wiki/Razze/Strix) | 5 | 12 | 10 |
| suli | [Suli](https://golarion.altervista.org/wiki/Razze/Suli) | 1 | 3 | 3 |
| svirfneblin | [Svirfneblin](https://golarion.altervista.org/wiki/Razze/Svirfneblin) | 2 | 3 | 3 |
| gillman | [Uomini Pesce](https://golarion.altervista.org/wiki/Razze/Uomini_Pesce) | 3 | 4 | 4 |
| vanara | [Vanara](https://golarion.altervista.org/wiki/Razze/Vanara) | 2 | 6 | 3 |
| vishkanya | [Vishkanya](https://golarion.altervista.org/wiki/Razze/Vishkanya) | 2 | 3 | 2 |
| wayang | [Wayang](https://golarion.altervista.org/wiki/Razze/Wayang) | 1 | 4 | 4 |

41 alternate entries represent 40 source bullets: Duergar's Dwarven Traits has two distinct replacement variants. All 69 favored options restrict the corresponding class enum; none grants eligibility or suppresses the ordinary HP/rank alternatives. Single-rate fractional rewards use selectionsPerBenefit; mixed-rate/initial-offset rewards explicitly remain manual.

## Implemented mechanics

- Ability modifiers, size/carrying profiles, land/swim/climb/fly speeds and flight maneuverability; slow-and-steady Duergar movement ignores armor/load.
- Ordinary and contextual skills, saving throws, attacks, maneuver defenses, natural armor, dodge, innate energy resistances, spell resistance, immunities and fire vulnerability.
- Two distinct Changeling claw grants and Kitsune bite, including physical damage types from [natural attacks](https://golarion.altervista.org/wiki/Attacchi_Naturali). Forms/limb occupancy are not inferred: the Kitsune bite remains present but has a marked reminder restricting use to its natural form.
- Samsaran chooses two distinct Knowledge skills with bonus and class-skill grants. Drow Noble Ambitious Schemer chooses Bluff or Diplomacy, granting the bonus and class-skill status to the selected skill. Suli selects its initial elemental language and Energy Strike element.
- Swim/climb native-speed elements include their +8 racial skill modifier, based on [Swim](https://golarion.altervista.org/wiki/Nuotare) and [Climb](https://golarion.altervista.org/wiki/Scalare). Taking 10, terrain and special movement rules remain marked reminders. Replacement removes the speed and its bonus together; Grippli ranger's threshold explicitly does not add a duplicate +8.
- All language allowlists reuse the reviewed nonsecret Linguistics list. Drow Noble additionally allows its explicitly listed Drow Sign Language for Intelligence choices only, not Linguistics; this is a racial exception, not a global secret-language unlock. Changeling host-society language remains manual; no arbitrary race inference.

## Explicit boundaries and exceptional replacements

- Changeling: baseline maternal heritage is a three-option choice. Alternatives replace that whole heritage element, because current replacement IDs cannot target one choice option. The required maternal ancestry is stated in a marked reminder and must be checked by the player; it is not enforced. Ocean's Daughter remains manual because the closed bonus enum has no trait-bonus category; no incorrect generic/racial substitute is installed.
- Duergar: Invisibility and Enlarge Person are distinct elements, so Blood Enmity removes only Invisibility. Deep Magic removes both. The open Dwarven Traits substitution is manual; its Hardy variant is a separate selectable alternative replacing Duergar Immunities with the actual conditional +2. Daysighted replaces both darkvision and light sensitivity.
- Suli Energy Strike replaces both the assault and the four-resistance element, as required by its body text, despite the source's final replacement line naming only the assault. The chosen resistance is real; activation, duration, added damage and elemental subpowers remain manual.
- Merfolk Strongtail replaces both speeds, preserving the native swim bonus. Gillman Throwback withdraws aquatic type, amphibiousness, swim speed/bonus/class skill and water dependency, replacing type/body metadata without removing land speed. Vanara Tree Stranger withdraws climb speed/bonus and grants every Knowledge class skill.
- Vision and creature-type adjudication, weapon proficiencies, casting-instance-specific bonuses, named spells absent from the spell dataset, limited-use powers/poison, companion/eidolon changes and open specializations are marked manual. No nonexistent spell/feat IDs are registered.
- Favored-class exceptions: Nagaji alchemist has mixed +1 alchemy/+1 per three poison DC rates; Svirfneblin ranger has an initial RD grant followed by half-step increments and a cap. Both preserve the rule in reminders rather than misusing a single common denominator. Grippli ranger installs the Swim check modifier but its movement unlock is manual. Strix slayer automates conditional Perception/Survival, with Stealth after Stalker manual. Vanara druid automates Handle Animal with Wild Empathy manual.
- No inferred Unchained class aliases or automatic prerequisites for borrowed traits. Golarion's wording controls content: e.g. no extra Svirfneblin Goblin bonus language or Wayang darkvision is imported from an English page.

## Verification

All 15 definitions pass the production JSON parser via a standalone temporary validator. Parent integration's `race_catalog_test` verified each baseline, alternate and choice, all favored rewards through their threshold, persistence and cleanup. The final integration rebuild includes localized movement labels, selected-skill bonus ownership, English IDs and claw damage types. No production backend changes or save-format changes are needed.
