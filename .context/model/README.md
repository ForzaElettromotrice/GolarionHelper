# Model Context

Release scope and Data/View verification for the first sheet backend version: [backend-v1.md](backend-v1.md).

- [`pending-choices.md`](pending-choices.md): typed completion reports, exact choice references and agreed advancement-blocking policy.

- [`operation-definitions.md`](operation-definitions.md): typed public operation parameters, shared Definition structs and separation from saved Data.

- [`enhancements.md`](enhancements.md): user-created groups of numeric contributions/modifiers, independent activation, sheet operations and format-36 persistence.

- [`archetypes.md`](archetypes.md): archetype compatibility, structural feature edits, shared choice APIs and format-31 persistence.
- [`archetype-catalog.md`](archetype-catalog.md): 661 included class archetypes, 764 reviewed source entries, 103 pending entries and explicit automation boundaries.

- [`class-catalog.md`](class-catalog.md): 44 playable Golarion class definitions, provenance, implemented progressions and marked manual boundaries.
- [`class-options.md`](class-options.md): in-progress class-feature option population, reusable catalog/binding schema and manual eligibility boundaries. Read before editing option data or attaching new class choices.

- [`classes.md`](classes.md): class definitions/runtime, sheet integration, level/choice progression, HP/skill-rank advancement, persistence and Views.
- [`favored-classes.md`](favored-classes.md): eligibility grants, per-level bonus choices, cumulative benefits, declarative effects and format-33 persistence.

- [`granted-spells.md`](granted-spells.md): single-spell grants, own/shared usages, item bindings, effect JSON and format-29 persistence.

- [`repertoire.md`](repertoire.md): source-owned known-spell quotas, prepared/flexible manual additions, grants, pending choices and format-40 persistence.
- [`spells.md`](spells.md): spell definitions/catalog, class-list selection, slot grants, all three casting modes, container/format-27 persistence, and research for single-spell grants.

- [`metamagic.md`](metamagic.md): typed applications, preparation/casting grant endpoints, verified rule distinctions, and pending runtime integration.

- [`languages.md`](languages.md): granted/learned languages, learning quotas, reminders, persistence, and pending racial integration.

- [`feats.md`](feats.md): per-character feat slots, selection, qualification, cleanup, and current runtime limitations. See [feat-catalog.md](feat-catalog.md) for the 1648-definition Golarion dataset and automation boundaries.

- [`feat-prerequisites.md`](feat-prerequisites.md): prerequisite value alternatives, source callbacks, and the boundary with slot eligibility and exemptions.

Read only the document for the subsystem being changed. Cross-cutting decisions remain in [`../architecture.md`](../architecture.md).

- [`declarative-content.md`](declarative-content.md): shared effect definitions and the current declarative feat boundary.
- [`effect-endpoints.md`](effect-endpoints.md): JSON endpoint coverage, typed callback schemas, runtime ID ownership and boundaries after the backend audit.
- [`races.md`](races.md): race catalog, runtime selection, choices, persistence, and View.
- [`special-defenses.md`](special-defenses.md): energy defenses, damage reduction, immunities, spell resistance, fast healing, and regeneration.
- [`foundation.md`](foundation.md): repository state and core `CharacterSheet` ownership.
- [`movement-and-load.md`](movement-and-load.md): movement, size, carrying capacity, and encumbrance.
- [`equipment.md`](equipment.md): item catalog, inventory, containers, money, and equipment.
- [`combat.md`](combat.md): abilities, strikes, damage, combat maneuvers, attack routines, configured attacks, and actions.
- [`checks-and-health.md`](checks-and-health.md): skills, saving throws, initiative, armor class, and hit points.
- [`resources-and-persistence.md`](resources-and-persistence.md): modifiers, resource infrastructure, Views, save data, and JSON persistence.
- [`conditions.md`](conditions.md): condition infrastructure and canonical condition behavior.
- [`interop.md`](interop.md): C interoperability, build targets, frontend boundary, and repository layout.
