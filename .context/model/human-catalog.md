# Human catalog coverage

Read when extending human race data or interpreting its automated versus manual benefits. Dataset review: 2026-11-13. The 21 alternate traits and 40 favored-class options for 36 classes listed on [Golarion's human page](https://golarion.altervista.org/wiki/Umano) are included; additional English-only traits and racial subtypes are not imported. English IDs follow its linked [Human reference](https://www.d20pfsrd.com/races/core-races/human).

## Favored-class bonuses

The permanent quality `human.favoredClassOptions` registers 40 source-owned options, each restricted to its corresponding class enum. It does not grant favored-class eligibility: the player must select that class and acquire its levels. Base HP/rank alternatives remain available. No implicit Unchained aliases are added.

Five options are automated: Druid's conditional Diplomacy/Intimidate, Inquisitor's conditional Perception, Occultist's ritual skill checks, Vigilante's Seamless Guise check and Warpriest's combat feat slot. The latter uses ordinary prerequisites, not an extra exempt slot.

The other 35 options preserve their benefit as a marked reminder and explicitly disclose manual handling in their description. These include casting-specific repertoire additions, companion benefits, class ability pools/talents, chosen maneuvers/energies and capped bonuses. No casting ID is guessed and caps remain manual. Every completed reward group emits its own reminder describing one whole benefit.

Fractional rewards use `selectionsPerBenefit` (2, 3, 4 or 6), so incomplete groups are visible in the View without prematurely applying an effect. Removing the human source withdraws its rewards while retaining selected IDs; restoring it reactivates them. Descriptions expose manual requirements even before a full group completes.

The human catalog test covers all 40 options: registration without automatic eligibility, rejection for the wrong acquired/favored class, cumulative thresholds, supported skill/feat effects, save/load and source removal/restoration.

## Alternate traits

All 21 entries have their declared baseline replacements. Selecting a manual-only entry still removes the replaced baseline benefits: a reminder does not implement its missing benefit. No new racial-runtime exceptions, automatic activations, cross-sheet mutations or unrestricted substitute feat slots were introduced.

User-approved convention: retain unsupported rules as reminders and append exactly `Ehi, appena hai tempo vedi di correggermi se necessario!` for future review. All human fallback reminders, including creature type, carry this searchable marker. It is a review flag, not a promise that every listed rule requires automation. Preserve each rule's explanation before the marker.

| Trait ID (human prefix omitted) | Implemented | Manual remainder |
| --- | --- | --- |
| dualTalent | Two distinct ability choices, replacements | None |
| heartOfTheSun | Conditional Fortitude modifier | Climate severity |
| heartOfTheFields | Description/reminder only | Open Craft/Profession specialization and bonus, daily fatigue exemption |
| heartOfTheStreets | Conditional Reflex/dodge modifiers | Crowd terrain |
| heartOfTheSnows | Conditional saves/checks, trip defense | Climate severity |
| heartOfTheSea | Swim modifier and class-skill grant | Sailor specialization, concentration, breath duration |
| heartOfTheMountains | Climb and conditional Acrobatics modifiers | Altitude acclimation |
| heartOfTheSlums | Skill modifiers | Disease reroll |
| heartOfTheWilderness | Scaling Survival and conditional Constitution check | Death threshold |
| piety | Religion modifier/class skill, invocation contexts | Activation and daily-use enforcement |
| psychicDefense | Conditional saves | Context supplied by caller |
| aquaticAncestry | Scaling Swim and class-skill grant | Breath duration and take-10 permission |
| giantAncestry | Maneuver bonuses and Stealth penalty | None |
| reptilianAncestry | Conditional saves | Context supplied by caller |
| heroic | Description/reminder only | Hero-point system |
| silverTongued | Social skill modifiers | Attitude-step limit |
| adoptiveParentage | Description/reminder only | Adoptive-race selection, starting-language replacement, familiarity/feat |
| rationalize | Conditional saves | Context supplied by caller |
| mixedHeritage | Description/reminder only | Second distinct Heart trait and replacement-conflict exception |
| eyeForTalent | Sense Motive modifier | Ability bonus on companion's sheet |
| focusedStudy | Description/reminder only | Level-gated Skill Focus acquisitions and specialization choices |

The five manual-only traits are not mechanically complete. They deliberately do not create bogus fixed choices or silently waive the normal replacement rules. Their unresolved manual choices currently do not produce typed PendingChoice entries. `adoptiveParentage` leaves the ordinary starting-language grant in place until a future implementation explicitly replaces it.

Situational labels are caller-selected contexts, not an automatic encounter evaluator. Union contexts avoid adding the same racial benefit twice (notably slippery-surface checks and overlapping save categories). Heart of the Snows follows Golarion's saving-throw wording; its linked English page specifies Fortitude for the cold-climate portion. Untyped bonuses use Generic, not an invented racial type where the source does not specify one.

## Human language options

Human grants Common and explicit Intelligence/Linguistics allowlists through existing effects. The lists contain 82 distinct IDs: 79 derived from [Linguistics](https://golarion.altervista.org/wiki/Linguistica) and its linked [Darklands](https://golarion.altervista.org/wiki/Linguaggi_delle_Lande_Tenebrose), [dead](https://golarion.altervista.org/wiki/Linguaggi_Morti_di_Golarion), [human](https://golarion.altervista.org/wiki/Linguaggi_Umani_di_Golarion), and [nonhuman](https://golarion.altervista.org/wiki/Linguaggi_Non_Umani_di_Golarion) language lists, plus `reptoid`, `rougarou` and `yaddithian` from their Golarion race pages. This is embedded race data, not a new global language manager or UI catalog.

- Exclude Druidic, Dark Folk and Sakvroth/Drow Sign Language because Golarion identifies them as secret.
- Keep the existing `common` ID for Taldane/Common; do not offer a duplicate Taldane purchase. `tien` is separate. Regional Common selection is not implemented.
- Deduplicate Senzar and languages appearing in multiple lists. Represent the named Varisian and Pathfinder sign-language traditions separately, rather than pretending every sign language is one language.
- Do not create entries for the generic Darklands-dialect category or Plantspeech: the former is not a single language, and the latter is described as spell-equivalent plant communication rather than an ordinary learned tongue. This does not grant physical or magical communication abilities.
- English IDs preserve existing conventions (`gnome`, `elven`, `dwarven`, `orc`). Further names use the English references linked by the four Golarion pages; do not import newer-edition additions from those English pages.
- Choosing a listed language still requires an unused slot and rejects already known languages. Source removal preserves learned choices and reports invalid selections as before. Other reviewed races now register their own starting/Intelligence languages and the shared nonsecret Linguistics list, except explicit marked quota or source omissions.

## Verification

`human_catalog_test` checks exact trait coverage, application/reversal of every entry, replacement conflicts, supported numeric/class-skill effects, language eligibility/quota enforcement, source withdrawal and save/load. Existing race/advancement tests cover the base human effects and Dual Talent. No production backend interfaces or save versions changed in this dataset step.

Sheet-only build and all 96 tests pass. No play tests were run.
