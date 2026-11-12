# Granted single spells

Read when changing separately granted spells, `spellGrant` effects or item charge bindings. Full casting systems remain in [spells.md](spells.md); item pools are in [equipment.md](equipment.md).

## Runtime and ownership

- `GrantedSpell` represents one catalog spell granted by a stable source ID. Closed `GrantedSpellUsage` distinguishes Limited, AtWill, Constant and SharedPool. This is usage bookkeeping, not a claim that every item activation is a spell-like ability.
- Limited owns a `UsagePool` with positive capacity and unit cost. SharedPool requires a live source-owned handle and positive cost no greater than capacity. AtWill/Constant have no pool; constant effects cannot be consumed as activations. No repertoire or class-list membership is needed. Unknown spell IDs and incompatible fields are rejected.
- `UsagePool` stores only capacity and remaining uses: `consume(amount)` validates before deduction, `refill()` fills completely, and `restore(amount)` adds a positive amount capped at capacity without overflow. There is no recovery category or timing enforcement. The user explicitly wants the reason/conditions in descriptive text, not in the pool.
- `GrantedSpells` owns the `spells.granted` typed collection for `GrantedSpellDefinition`. Removing an entry revokes the ability; an external handle to a pool does not authorize casting. Re-adding a Limited grant starts a new own counter. Shared item counters survive activation cycles because the item owns them. No inactive-grant history is maintained.
- The container exposes consume/refill/restore by grant ID, Data and a detached View of ID/source/spell/usage/capacity/remaining/cost. Only Limited pools may be refilled/restored through the spell API; shared reserves are manipulated through their source. AtWill consume is unlimited; Constant consume rejects. Consumption now exports `SpellCastResult` using the container's caster registry and catalog after spending uses. Export failures do not refund charges. Target application, caster level/save DC rules, item activation requirements and spell-like-specific feat interactions remain separate; see [spells.md](spells.md).
- `CharacterSheet` constructs the container before feats/race/inventory, includes its View and forwards `consumeGrantedSpell`, `refillGrantedSpell`, `restoreGrantedSpell`. It also forwards item refill/restore to Inventory. The registry outlives all providers; container and spells cannot be copied.

## Declarative effects

`spellGrant` uses stable English JSON names and a closed usage object:

```json
{"id":"magic","type":"spellGrant","spellId":"fireball","usage":{"type":"limited","capacity":2}}
```

Other usage objects are `{"type":"atWill"}`, `{"type":"constant"}`, or `{"type":"sharedPool","poolId":"charges","cost":2}` (cost defaults to one). Unknown types, irrelevant usage fields, missing required fields, fractional/overflowing/invalid counts reject.

- `compileEffect` grants through the collection using the owning race/item's existing stable effect instance ID and returns exact cleanup. Source metadata is supplied by the owner, never invented by the parser.
- `EffectContext` optionally resolves source-local usage pool IDs. Item compilation captures its own pool handles by value, so moving an ItemInstance cannot leave dangling references. Direct and selected-choice item effects use the same path. A missing pool/resolver fails; no implicit reserves are created.
- Limited item effects must reference declared item pools via SharedPool, rather than owning counters that would reset on equip. AtWill/Constant item effects need no reserves. Racial Limited effects use the ordinary generic compiler; no race catalog expansion was made.
- No full `spellcasting` creation effect was added here. That larger subsystem's concrete providers remain deferred.

## Persistence

- Sheet format 29 adds `grantedSpells`, storing only ID and remaining count for Limited grants. Shared item counts remain solely in inventory data; unlimited/constant state and grants are reconstructed from sources, not duplicated.
- Load occurs after source reconstruction and prerequisite reconciliation. It validates all owned counters and exact IDs before swapping any state; unknown/missing/duplicate IDs, wrong ownership and out-of-range values fail atomically. It never spends or replenishes an external pool. Pre-29 saves have no own granted-spell state and use reconstructed initial values.
- Tests cover all modes, shared spending, source cleanup, empty/invalid permissions, malformed effect JSON, atomic state loading, item activation/re-activation and actual sheet file round trips. Catalog-dependent tests use an isolated item fixture, not production catalog entries. All 70 sheet tests pass; `core/play` was not built or tested.

## Rules research and decision

- [Gnome magic](https://golarion.altervista.org/wiki/Gnomo) supplies daily abilities; [noble drow](https://golarion.altervista.org/wiki/Razze/Drow_Nobile) demonstrate daily, at-will and constant abilities.
- [Cerca Cielo](https://golarion.altervista.org/wiki/Cerca_Cielo) grants weekly Commune. [Wands](https://golarion.altervista.org/wiki/Bacchette) have finite total charges, whereas [staves](https://golarion.altervista.org/wiki/Bastoni) can regain charges through specific rules.
- Confirmed user decision: these differences belong in descriptions; the model allows explicit refill/restore without interpreting why or when. Constant effects remain distinct from repeated unlimited activations.
