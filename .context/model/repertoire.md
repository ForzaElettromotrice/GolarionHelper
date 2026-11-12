# Spell repertoires

Read when changing known-spell quotas, manual additions, grants, completion reports or repertoire saves. Daily casting/preparation slots remain separate; see [spells.md](spells.md).

## Runtime and APIs

- Each `Spellcasting` owns an initially empty `SpellRepertoire`. It resolves standard and sheet-local custom spell definitions through the casting's catalog.
- `selectRepertoireSpell(RepertoireSelectionDefinition{quotaId, spellLevel, spellId})` consumes one available selection of that source/level. The spell must exist, have exactly that level on the casting list, and match the optional shared ID filter. One spell cannot occupy multiple quota selections. `removeRepertoireSelection` frees the choice without removing its quota, including after the quota source disappears.
- Prepared and flexible casting also expose manual `addToRepertoire`/`removeFromRepertoire`, independent of quotas. IDs must resolve in the catalog; additions are idempotent. List/level authorization remains part of preparation. Spontaneous casting rejects these manual operations. Sheet forwarding methods recheck feat prerequisites.
- Different origins can overlap: removing a manual entry or a source does not remove another origin's knowledge. Explicit grants can contain off-list spells, but casting them still needs existing level/list overrides or other appropriate casting authorization.
- Removing a quota retains its selections as inactive; regranting the same stable ID can reactivate them. If a source shrinks below the saved number of selections at one level, all selections at that level become inactive until the user removes the excess. Filter changes can likewise leave retained inactive choices. No arbitrary spell is silently deleted.
- Historical preparations survive subsequent loss of knowledge, including save/load. New preparations and spontaneous consumption still require current knowledge. Legacy preparation records without historical evaluations retain their original validation.

## Source endpoints and effects

All three endpoints are scoped by casting ID and unregister with the casting. Providers must exist before user selection and remove dependent effects before withdrawing the casting itself.

| Helper suffix after `spellcasting.<id>.` | Entry | Declarative effect |
| --- | --- | --- |
| `repertoire.quotas` | `SpellRepertoireQuota`: source ID, label, positive capacities by level 0–9, optional allowed IDs | `spellRepertoireQuota` |
| `repertoire.grants` | `SpellRepertoireGrant`: source ID, label, nonempty spell IDs | `spellRepertoireGrant` |
| `repertoire.lists` | `SpellRepertoireListGrant`: source ID and label; grants the instance's entire selected list | `spellRepertoireList` |

Quota JSON uses `castingId`, `quotas: [{level, capacity}]`, and optional `allowedSpellIds`. Grant JSON uses `castingId` and `spellIds`; list JSON uses `castingId`. All include an ordinary effect `id`. Parser, writer and reversible compiler support these effects, including favored-class benefits. The runtime grant ID comes from `EffectContext.instanceId`, not the local definition ID. Full-list access is dynamic, including later registered custom spells. No production class tables are installed automatically.

## Data, View and pending choices

- Format 40 stores a repertoire object with `manualSpellIds`, `selections` (quota ID, level, spell ID), and `unassignedSpellIds`. Structural quotas/grants/lists are rebuilt from sources, never serialized as player state. Container loading stages validation before committing all casting states and source balances.
- Legacy flat prepared/flexible knowledge becomes manual entries. Legacy spontaneous knowledge is preserved as unassigned, inactive IDs until explicitly selected into available quotas; migration does not invent unlimited grants. Successful selection removes the matching unassigned record. `SpellcastingData.repertoireData` is authoritative when present; its older `repertoire` field is retained for legacy input.
- `SpellcastingView.repertoire` contains explicit active IDs (manual, quota-selected, directly granted). It does not enumerate whole-list access. `repertoireDetails` exposes manual permission, manual/unassigned IDs, every retained selection and its active flag, quotas with remaining counts, direct grants and full-list grants. Consumers must account for list grants when presenting available spells.
- `pendingChoices()` returns a blocking `RepertoireChoiceReference{castingId, quotaId, spellLevel}` for each positive remaining count. This includes the prepared caster's free selections and excludes manual additions. Unfilled quotas prevent +1 in any class; manual knowledge does not fill them. Removing a selection reopens the blocking choice. Preparation of daily slots, -1 and loading retain their existing policies.
- Focused tests cover quota limits, filters, source overlap/removal/regrant/shrink, manual restrictions, dynamic list access, custom definitions, effect round trips, saved provenance, legacy reassignment and blocking references. Sheet integration tests reconstruct quotas through class effects before restoring selections.

Verification: build succeeds and all 92 sheet tests pass (`ctest --test-dir build/core/sheet --output-on-failure -j 4`). No `core/play` tests were executed.
