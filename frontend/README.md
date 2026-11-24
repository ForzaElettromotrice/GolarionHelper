# Sheet UI components

Reusable Qt Quick/QML subsystem panels for the Windows/Linux desktop application. Panels do not create a controller, save sheets or contact a game session. Current panel contracts and interactions are documented in [the frontend context](../.context/frontend.md).

## Desktop build and panel tests

All info expansions (abilities, skills and saving throws) render through `InfoDetails.qml`. Hosts provide typed rows using its `LineType` enum: Heading, Group, Entry, Total, Note or Spacer. The component alone owns typography, padding, indentation and HTML escaping; `summaryText` is a plain-text representation for testing. `ModifierText.rows` prepares grouped active modifier data without styling or calculations. SkillDetails is only a SkillView-to-rows adapter, not a separate renderer. Change info styles in InfoDetails to update all three panels.

Requires CMake 3.25+, a C++20 compiler and Qt 6.9+ with Quick, Quick Controls 2 and (for tests) Quick Test. Qt is dynamically linked; the project's own QML module is a static library/plugin.

```sh
cmake -S . -B build -DGOLARION_TARGET=golarion_helper -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/frontend/app/golarion_helper
```

With a multi-configuration Windows generator, pass `--config Debug` to build and `-C Debug` to CTest. Point `CMAKE_PREFIX_PATH` at the matching Qt kit if it is not discovered automatically.

Qt is not required for a backend-only build.

When running a development build or its tests on Windows, add the selected kit's `bin` directory to the current terminal's `PATH` so Windows can load its Qt DLLs.

If Qt is missing on native Linux x86-64 or Windows MSVC 2022 x64, CMake downloads pinned Qt 6.9.3 desktop archives from the public Qt repository, checks their SHA-256 hashes and extracts the kit into the build tree under `_deps/qt/6.9.3/`. No account, installer, Python or administrator privileges are needed. A compiler and Qt's platform dependencies must already be available. For offline builds, use an installed or previously extracted kit and set `GOLARION_ALLOW_DOWNLOADS=OFF`. For other platforms, or an incomplete system Qt installation, point `CMAKE_PREFIX_PATH` to a complete compatible SDK in a fresh build.

## IdentityPanel contract

- Import `Golarion.Sheet`. The panel has no window, controller or scrolling container of its own. Its parent supplies width and scrolling; height follows content.
- Supply a complete, immutable `identity` object. Missing fields initialize as empty. Replace the object to publish an update; do not mutate its nested properties in place.
- Fields match `CharacterIdentityView`: `name`, `playerName`, `alignment`, `deity`, `homeland`, `gender`, `age`, `heightCentimeters`, `weightGrams`, `hair`, `eyes`, `appearance`.
- Alignment uses the panel's closed QML enum, including `IdentityPanel.Unspecified`. Its numeric values are **not** the backend enum values: the future adapter must explicitly map them and map `Unspecified` to `std::nullopt`.
- Optional text/numeric values are emitted as `null` when cleared. Name/player name remain strings. Age is a nonnegative integer; height is a positive integer in cm. Weight is edited in kg (comma or dot, at most three decimal places) and emitted as exact integer grams within JavaScript's safe-integer range. Wider backend `int64_t` values require explicit handling at the future adapter boundary.
- `identityChangeRequested(proposedIdentity)` emits a complete proposal only on Apply. It never changes the input object. The parent can accept by replacing `identity`, reject via `errorMessage`, or mark an asynchronous operation with `busy`.
- `readOnly` prevents edits; `busy` prevents editing/submission/reset. `dirty`, `valid` and `canApply` expose form state.
- Drafts survive rejection. A conflicting incoming snapshot preserves the draft and blocks Apply until the user reloads. A confirmed snapshot matching the proposal resets the draft. Cancel always reloads the latest input.
- Empty alignment is allowed in this editor; the backend still decides advancement eligibility. No race/class/level fields or general sheet notes are duplicated here.

The desktop controller applies changes and publishes updated sheet state.

## AbilityScoresPanel layout

The detail toggle is now a borderless info icon at the end of each row; the dark ability-name cell is no longer a button. InfoButton is shared with skill and saving-throw panels.

The compact table matches the user-provided reference: six rows with dark abbreviation/name cells, score and modifier boxes, without a temporary column. Clicking its info icon toggles a detail section immediately below its row, showing only active score bonuses grouped by the closed BonusType enum, with signed values and sources beneath each heading. Penalties have a separate group. Conditions remain visible. Only the authoritative final total is shown, without category subtotals; active does not imply cumulative. Details use `AbilityView.modifiers` and the panel's closed `ModifierKind` enum. Missing detail data and empty modifier lists have explicit messages. The score cell normally displays the total. Hovering reveals an editable base value; Enter or focus loss after a click submits a positive C++ int through `baseValueChangeRequested(type, value)`. Escape cancels. Invalid drafts are discarded; a changed incoming base prevents stale edits from being submitted. The input snapshot is never mutated.

Supply an immutable `abilities` list with the closed panel `AbilityType` enum, `baseValue`, `totalValue` and `modifier`. Derived values are displayed unchanged. Missing records display a dash. The table has a preferred width of 340 pixels and shrinks with its parent. The future controller remains separate.

## SkillsPanel contract

Info panels use two compact sections, Gradi and Punteggio, with signed value/source lines and authoritative totals. Granted ranks are shown as alternatives to invested ranks, not summed. Multiple ability options retain separate score breakdowns.

## SavingThrowsPanel contract

Supply immutable `savingThrows` records matching SavingThrowView: closed `SavingThrowType` enum, `baseValue`, `baseContributions`, `abilityOptions` and `modifiers`. Rows always appear as Tempra, Riflessi, Volontà, with supplied scores (distinct alternatives separated by `/`) and a borderless info toggle. No rank/class-skill controls or editing. Details show base contributions, ability modifiers, grouped active bonuses/penalties and authoritative totals; text is escaped before styling. ContributionView currently provides an ID rather than a source label, so the detail shows that ID without guessing its meaning. Modifier formatting is shared with skill details.

Each concrete row has an info button expanding `SkillDetails` beneath it. Details use SkillView's `rankGrants`, `effectiveRanks`, `classSkillBonus`, `appliedArmorCheckPenalty` and each `abilityOptions` entry (ability enum/modifier, source, already-calculated total and modifier set). Active bonuses are grouped by type; no stacking rules or totals are recalculated. Trained-only unusability is explained. Category headers have no generic calculation detail. The footer displays host-supplied `spentRanks / maxRanks` (total entitlement, not remaining ranks), with dashes when unavailable.

Supply immutable SkillView-like `skills`: `type`, `specializationId`, `resourceName`, Italian `name`, `classSkill`, `ranks`, `usable` and `abilityOptions[].totalValue`. Specialized `name` is the specialization name alone, as supplied by SkillView; the panel prefixes its category. The panel sorts a copy by Italian name and resource ID. Columns are class-skill checkbox (indicator only), name, score and assigned ranks. Unusable skills show a dash; distinct alternative scores are separated by `/`, without choosing an ability or recalculating rules. Rank editing is deferred.

Three category rows always provide a text field beside Craft/Perform/Profession (Italian labels). Enter on a nonblank trimmed name emits `specializationAddRequested(skillType, name)` without mutating input. The closed QML `SpecializationType` enum must be explicitly mapped by the future adapter to backend SkillType. The controller owns ID generation, duplicate handling and backend validation. The demo alone accepts requests into synthetic state. Canonical specializations, including Alchemy, already exist at backend initialization; effects do not auto-create missing custom specializations.

Category buttons expand/collapse their specialization rows independently, initially collapsed. All supplied specializations are shown alphabetically when open. Expansion survives snapshot replacement; submitting a specialization opens its category. The input remains available while collapsed.

Each specialization has a pin toggle: pinned rows remain visible when their category is collapsed. `pinnedResources` is view-local state keyed by resourceName, retained across snapshots and pruned when rows disappear; persistence is deferred. Only records with `custom: true` show the remove ×, which emits `specializationRemoveRequested(skillType, specializationId)` without mutating input. The demo accepts custom removals; the future controller must call the existing backend removal API and report rejections (e.g. active grants). Canonical specializations cannot be removed.

The class-skill checkbox appears on each category header, not on its specialization rows. Its state comes from the category record (`type`, no `specializationId`, `classSkill`) supplied by the host, not from whether an individual specialization is a class skill. The current backend SkillView list has only concrete specializations: the future adapter must supply these category records. Checkbox interaction and backend custom grants to whole categories remain separate work.

Specialization labels have a 20px left indent. Concrete rank cells display authoritative `effectiveRanks` normally and an editable player-assigned `ranks` value on hover. Enter or focus loss after a click emits `ranksChangeRequested(resourceName, ranks)` for nonnegative C++ int values; Escape or invalid input restores the snapshot. The panel neither sums granted ranks nor recalculates scores. No generic category score is currently exposed by SkillsView.
