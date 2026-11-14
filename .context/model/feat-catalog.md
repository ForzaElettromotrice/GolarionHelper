# Feat catalog

Read before changing standard feat definitions or claiming that a listed talent is fully automated.

## Scope and sources

Reviewed 2026-11-13 using only Golarion's [general database](https://golarion.altervista.org/wiki/Database_Talenti), [setting database](https://golarion.altervista.org/wiki/Talenti_di_Golarion) and individual feat pages. The union contains **1648 ordinary Pathfinder feats**, including the existing `improvedInitiative`. Excluded: **159 mythic** and **70 explicitly legacy 3.5** entries. All selected pages were readable; there is no missing-page pending list for this pass.

Runtime data is embedded from `resources/feats.json` and `resources/feats/{general,combat,other}.json`. Combat-tagged definitions are grouped together, even when they have additional types. The other grouping contains the remaining non-general categories; categories are retained, not inferred from the file name. CMake tracks all four files as configure dependencies. Explicit string-view length avoids constexpr string scanning limits.

`resources/feats/catalog.sources.json` is a development provenance index, not a runtime input. It records all IDs, Golarion URLs and authoring review notes. Italian descriptions are short paraphrases with a source link, not the full rules. English IDs follow Golarion's original-name column or its linked-reference terminology when the column is demonstrably wrong; no English-only rules are imported.

`FeatType::Alignment` / JSON `alignment` covers the nine Allineamento feats, including codec round trips and typed feat-slot filters. No general-category fallback or open category strings are introduced. Manager APIs and persistence format are unchanged.

## Prerequisites

- **628 numerical requirements** use existing character ability, BAB and total-level targets. Expressions are resolved by the existing feat prerequisite registry, so source alternatives still work. Class levels and caster levels are not substituted with total character level.
- **626 feat dependencies** refer to existing catalog IDs, with no self-dependencies or directed cycles. The importer accepts only unambiguous complete prerequisite clauses linked to the corresponding talent page.
- **1908 manual clauses** preserve the source requirement and the exact review marker. They do not block acquisition or activation, by the user's existing decision. They also generate source-owned reminders so the unverified conditions remain visible after acquisition.
- An OR, conditional or alternative prerequisite block is not flattened into an AND. Ambiguous combinations remain manual. A dependency on a specific weapon/skill choice is not replaced with an unrestricted dependency on any acquisition of that feat.
- Skill ranks are not currently exposed as ExpressionParser targets. No nonexistent `skill.*.ranks` targets were invented. Racial/class/deity eligibility, equipment proficiency, caster-level requirements and open choices remain manual where existing targets do not represent them.

## Effects and choices

**46 feats** provide supported mechanical effects, including Improved Initiative, Toughness, static saves, skill pairs, combat maneuvers, dodge/shield defenses, running and contextual Power Attack/Deadly Aim. Unsupported remainder and manual clauses end with `Ehi, appena hai tempo vedi di correggermi se necessario!`.

- Toughness contributes to `hp.max` and scales with total character level as modeled by the sheet. It does not infer unmodeled racial Hit Dice.
- Skill-pair bonuses have their actual base +2 plus a conditional +2 for the individual skill at ten ranks. The player selects that context; the catalog does not automatically inspect nonexistent rank targets.
- Power Attack evaluates BAB dynamically on its owner. Its ordinary, two-handed and off-hand/secondary-natural damage contexts are alternatives, never simultaneous bonuses. Activation and expiry at the next turn remain player/action-manager responsibilities. Deadly Aim similarly excludes touch attacks through its explicit context.
- Steel Soul adds only +2 above the existing Hardy +2, not another +4. Its condition requires the original racial benefit to exist. Improved Drag's Golarion page names trip defense rather than drag defense; the offensive bonus is real while the disputed defense stays marked manual.
- Weapon-specific benefits, casting-instance-specific grants, companion/eidolon changes, additional class powers, event resolution, daily-use accounting and choices not representable by the current model remain marked manual. An ability granted to a companion is not applied to the owning character.
- Skill Focus and other open weapon/skill-specialization choices are not restricted to an incomplete finite catalog. No Craft/Perform/Profession-wide modifier substitutes for one specialization.

## Repeated acquisition

**24 feats** explicitly support `unlimited` acquisition: the source permits additive repetition without requiring a distinct choice or an unmodeled limit. Conditional notes still apply, e.g. Third Eye repeats uses rather than every benefit.

**99 other repeatable feats** remain conservatively `once` with a visible reminder because repetitions require different open choices, class-level intervals or acquisition caps not represented in this pass. Do not claim these repetitions are enforced correctly. Do not change them to `unlimited` merely to bypass missing choices. Existing runtime support for `differentChoices` remains available when a complete valid choice model can be supplied.

## Source discrepancies

- Channeling Force and Channel Force have different Golarion pages despite sharing the Italian title Incanalare Forza. Improved Share Spells and teamwork Improved Spell Sharing are also distinct. Their canonical English IDs are not merged. Lancio Parabolico uses Arcing Lob rather than the erroneous repeated Giantbane Caster name in the index.
- Sguardo Intimidatorio has incompatible index rows. The actual page describes the mesmerist combat feat; the obsolete inquisitor/general row is not imported as a second invented definition.
- Rending Fury's page reduces the number of attacks needed, not damage; Flame Warrior/Storm Warrior use fire/electricity respectively despite swapped table summaries. Fanged Crown Massacre modifies an ally's attack, not the save described by its erroneous summary row. Source details take precedence over table shorthand.

## Verification

`feat_catalog_test` covers unique IDs, types, complete JSON round trips, manager lookup, reminder markers, dependency identity, acquisition through user-owned extra slots, save/load and effect removal for every definition and each represented choice. Focused checks cover normal-slot Power Attack eligibility and deactivation/reactivation, Toughness at levels 0/5/10, and the exact Reflex-save increase. Catalog-wide dependency checks find no missing IDs or cycles.

Final verification on 2026-11-13: the sheet-only build and all **98 sheet tests pass**, including acquisition, persistence and cleanup across all **1648 definitions**. Focused scaling tests use an explicit ten-level synthetic class progression; Toughness targets the actual `hp.max` endpoint. Every definition has a matching Golarion provenance record, all structured dependencies resolve without cycles, and `git diff --check` passes. No play tests, commit or push performed.
