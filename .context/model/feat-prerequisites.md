# Feat Prerequisites

Read this document when working on feat qualification, slot-specific exceptions, or the future per-character feat runtime.

## Current Implementation

- `FeatPrerequisiteRegistry` registers the typed `feat.prerequisiteAlternatives` collection with `ResourceManager`. Source-owned entries contain a normalized ID, source, target name, applicability callback, and calculation callback. Duplicate IDs and empty callbacks are rejected; removing an entry removes only that source's alternative.
- `FeatPrerequisiteContext` supplies the feat definition, slot ID/source, and selected option IDs grouped by choice. Applicability may depend on feat types, exact feat IDs, slots, or choices.
- Calculation callbacks receive a read-only target resolver backed by the original `ResourceManager`. Reading `bab` there reads the sheet's actual BAB, not another alternative. Callbacks must not mutate or reenter the registry.
- `values()` returns the ordinary value followed by applicable alternatives in stable ID order, preserving source/entry provenance. `satisfies()` tests a caller-provided predicate against those candidates with OR semantics. Values do not add automatically, and evaluation does not merely select the maximum. Explicit combinations belong to the source callback.
- Targets must exist in `ResourceManager`; unknown targets and callback errors propagate instead of silently qualifying a character. Values are recalculated on demand.
- The registry is owned by the sheet's `Feats` component and is also independently testable. It has no persistence or public C API.

## Agreed Boundaries and Next Integration

- Removing a selected feat leaves its slot available. Withdrawing the slot is a separate source-owned operation.
- Check slot eligibility first, then apply source-granted exemptions at the higher qualification layer, then check remaining prerequisites. Exemptions are not entries in this value registry.
- All required prerequisites must pass; OR applies to permitted alternatives for an individual check. `Feats` evaluates complete expressions with consistent values for repeated target references and candidate combinations across different targets. Stable-state revalidation and structured feat dependencies are implemented; manual text requirements are ignored by explicit user decision. See [feats.md](feats.md).
- Bonus-slot context matters: the Priest's BAB equivalence is not a blanket rule for all combat feats. Sources: [Sacerdote Guerriero](https://golarion.altervista.org/wiki/Sacerdote_Guerriero), [Attaccabrighe](https://golarion.altervista.org/wiki/Attaccabrighe), [Kensai](https://golarion.altervista.org/wiki/Magus/Archetipi#Kensai).
