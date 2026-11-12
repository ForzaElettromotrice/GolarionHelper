# Enhancement groups

Read when changing user-created or received effects, group lifecycle, sheet APIs or saved enhancement state.

## Definitions and ownership

- `EnhancementGroupDefinition` contains ID, display name, ordered `EffectDefinition` entries, one `active` flag (default true), optional description and optional provenance.
- `EnhancementOrigin` uses the closed `EnhancementOriginType` enum (Custom, Spell, Item, Ability), optional definition ID and source-sheet ID. Provenance is informational and grants no permissions.
- Individual effects no longer have an active flag or a separate EnhancementDefinition/Data wrapper. There is no per-entry activation API.
- Runtime registration uses the shared `compileEffect` pipeline and its cleanup callbacks. Group/entry IDs are namespaced with length prefixes. `enhancementEffectId(groupId, effectId)` gives the runtime ID for explicit references between effects. Display names never identify registrations.
- No extra destination/type allowlist is introduced. Shared schema/compiler and endpoint validation still apply; a source-dependent effect without its required runtime provider/context fails rather than bypassing validation.

## Lifecycle

- `CharacterSheet` exposes add/removeEnhancementGroup, add/removeEnhancement and `setEnhancementGroupActive(groupId, active)`. Each successful mutation rechecks feat prerequisites.
- Add/remove individual effects is allowed only while the group is inactive. Activation applies the entire ordered sequence; deactivation and destruction clean up in reverse order. Repeated group activation/deactivation is idempotent.
- A failed activation rolls back completed registrations and leaves the group inactive. Failed creation does not publish the group. Definitions are structurally validated while inactive; unavailable resources and live numeric evaluation are deferred to activation.
- Numeric contributions/modifiers retain evaluation, negative-magnitude and circular-expression checks at activation. They retain ordinary live-expression and stacking behavior.
- Keep dependent effects inside the same group and declare providers before consumers. External providers must outlive consumers, as for other source effects. There is no dependency graph across independent sources and no automatic ordering inferred from arbitrary resource strings.
- Groups preserve insertion order in runtime, Data and View; the container destroys them in reverse insertion order. This avoids alphabetic reordering of declared effects or reconstruction.
- By user decision, toggling may reset state owned by recreated grants (uses, slots, temporary pools). No special state preservation or resource allowlist is added.

## Data and View

- Save format 42 stores group `id`, `name`, `active`, `description`, `origin` and ordered `effects`, using the shared effect JSON codec.
- The old per-entry enhancement schema is rejected without migration. No existing files are deleted. Pre-existing compatibility code for unrelated save fields remains outside this change; saves containing enhancements require the new version/schema.
- `EnhancementGroupData` is distinct from the creation definition. Views expose group state/metadata and the effect definitions; `resolvedValue` is present only for active numeric modifiers/contributions, not for nonnumeric or inactive effects.
- Sources restore before feats and mutable spell/HP state, allowing enhancements to supply feat slots and casting instances before their saved selections are loaded.

## Spell integration boundary

- Spell targeting, recipient authorization and application belong to the play/application layer, not this subsystem.
- The game session's sheet manager will apply exported spell effects as recipient groups; no automatic recipient application remains to implement in the sheet backend.
- Spell consumption now exports values such as caster `@level` as concrete literals in copied definitions. No live caster link is introduced. Relative rules such as bounded movement remain recipient-side. Automatic recipient/group application remains separate; see [spells.md](spells.md).
- Effects received from a spell can use one group per application, including self-targeting. Instant damage/healing is not represented as a reversible group.

## Verification

The sheet-only build and all 93 sheet tests pass. Focused enhancement tests cover group-only toggling, inactive-only editing, declared effect order, reverse cleanup of casting/slot dependencies, rollback, numeric validation, source isolation/stacking, nonnumeric grants, metadata, JSON rejection of the old shape, round trips and feat activation after slot withdrawal/regrant. No play tests were executed.
