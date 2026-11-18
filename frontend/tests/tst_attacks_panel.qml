import QtQuick
import QtTest
import Golarion.Sheet
TestCase {
    id: testCase
    name: "AttacksPanel"
    width: 500; height: 600
    visible: true
    when: windowShown
    Component { id: component; AttacksPanel {} }
    Component { id: detailsComponent; AttackDetails {} }
    function test_damageAbilityContributions() {
        const details = createTemporaryObject(detailsComponent, testCase, {attack: {
            strike: {},
            normalDamage: {abilityOptions: [{abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 2, bonus: 5}]},
            criticalDamage: {abilityOptions: [{abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 4, bonus: 10}]}
        }});
        verify(details.summaryText.includes("Danni\n+2 Forza"));
        verify(details.summaryText.includes("Danni critici\n+4 Forza"));
        verify(!details.summaryText.includes("+5 Forza"));
        verify(!details.summaryText.includes("+10 Forza"));
        details.attack = {strike: {}, normalDamage: {abilityOptions: [
            {abilityType: AbilityScoresPanel.Strength, abilityModifier: -2, abilityContribution: -2, bonus: 1}
        ]}};
        verify(details.summaryText.includes("-2 Forza"));
    }
    function test_attackDetails() {
        const details = createTemporaryObject(detailsComponent, testCase, {width: 240, baseAttackBonus: {total: 11}, attack: {
            routineAttackBonusAdjustment: -2, progressionAttackBonusAdjustment: -5,
            strike: {source: "Arma <test>", defenseOptions: [{defenseType: ArmorClassPanel.Touch}],
                reach: {minimumUnits: {effectiveValue: 0}, maximumUnits: {effectiveValue: 2}},
                usageAttackPenalty: 0,
                attackAbilityOptions: [{abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, attackBonus: 9, criticalConfirmationBonus: 12, source: "Base"}],
                attackModifiers: {modifiers: [
                    {active: true, type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, resolvedValue: 1, source: "Magia"},
                    {active: false, source: "Non mostrare"}]},
                damageComponents: [{source: "Arma", effectiveDice: {expression: "1d8"}, types: [DefensesPanel.Slashing], typeMode: AttackDetails.All}],
                requirements: [{requirement: {satisfied: false, failureReason: "Requisito mancante"}}]
            }
        }});
        verify(details.summaryText.includes("CA di contatto"));
        verify(details.summaryText.includes("0 m – 3 m"));
        verify(details.summaryText.includes("Tagliente"));
        verify(details.summaryText.includes("+1 Magia"));
        verify(details.summaryText.includes("+11 BAB"));
        verify(details.summaryText.includes("+4 Forza"));
        verify(details.summaryText.includes("-2 Routine"));
        verify(details.summaryText.includes("-5 Progressione"));
        verify(details.summaryText.includes("Totale: +9"));
        verify(details.summaryText.indexOf("+11 BAB") < details.summaryText.indexOf("Totale: +9"));
        verify(!details.summaryText.includes("Totale ("));
        verify(!details.summaryText.includes("Non mostrare"));
        verify(details.summaryText.includes("Requisito mancante"));
        verify(details.text.includes("&lt;test&gt;"));
    }
    SignalSpy { id: addSpy; signalName: "addRequested" }
    function test_damageSummaryOmitsTypes() {
        const panel = createTemporaryObject(component, testCase, {attacks: {attacks: []}});
        const physical = {occurrences: 1, component: {effectiveDice: {expression: "1d8"}, types: [DefensesPanel.Slashing], typeMode: AttackDetails.All}};
        compare(panel.damageSummary({components: [physical], abilityOptions: [{bonus: 4}]}), "1d8 +4");
        const fire = {occurrences: 1, component: {effectiveDice: {expression: "1d6"}, types: [DefensesPanel.Fire], typeMode: AttackDetails.All}};
        compare(panel.damageSummary({components: [physical, fire], abilityOptions: [{bonus: 4}]}), "1d8 + 1d6 +4");
        const choice = {occurrences: 2, component: {effectiveDice: {expression: "1d8"}, types: [DefensesPanel.Slashing, DefensesPanel.Piercing], typeMode: AttackDetails.Choice}};
        compare(panel.damageSummary({components: [choice], abilityOptions: [{bonus: 8}]}), "2 × (1d8) +8");
    }
    SignalSpy { id: assignSpy; signalName: "assignRequested" }
    SignalSpy { id: removeSpy; signalName: "removeRequested" }
    function test_removalLock() {
        const snapshot = {attacks: [{id: "first", name: "Routine", actionId: "blocked"}]};
        const panel = createTemporaryObject(component, testCase, {attacks: snapshot, width: 260,
            actions: {categories: [{actions: [{id: "blocked", usable: false}]}]}});
        removeSpy.target = panel;
        removeSpy.clear();
        waitForRendering(panel);
        const lock = findChild(panel, "routineRemovalLock");
        const remove = findChild(panel, "remove_first");
        verify(!remove.visible);
        mouseClick(lock);
        verify(lock.checked);
        verify(remove.visible);
        verify(remove.enabled);
        waitForRendering(panel);
        mouseClick(remove);
        compare(removeSpy.count, 1);
        compare(Array.from(removeSpy.signalArguments[0]), ["first"]);
        compare(snapshot.attacks.length, 1);
        verify(!findChild(panel, "routine_first").checked);
        mouseClick(lock);
        verify(!remove.visible);
        compare(removeSpy.count, 1);
    }
    function test_addRoutineMenu() {
        const available = {routines: [
            {id: "single", name: "Singolo <test>", usable: true},
            {id: "blocked", name: "Bloccata", usable: false, failureReasons: ["Azione impedita"]},
            {id: "full", name: "Completo", usable: true}
        ]};
        const original = JSON.stringify(available);
        const panel = createTemporaryObject(component, testCase, {attacks: {attacks: []}, attackRoutines: available, width: 260});
        addSpy.target = panel;
        addSpy.clear();
        waitForRendering(panel);
        const button = findChild(panel, "addRoutine");
        mouseClick(button);
        const menu = button.routinesPopup;
        tryCompare(menu, "opened", true);
        compare(menu.width, 260);
        compare(menu.itemAt(0).text, "Singolo <test>");
        verify(!menu.itemAt(1).enabled);
        mouseClick(button);
        tryCompare(menu, "visible", false);
        compare(addSpy.count, 0);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        mouseClick(menu.itemAt(2));
        compare(addSpy.count, 1);
        compare(Array.from(addSpy.signalArguments[0]), ["full"]);
        tryCompare(menu, "visible", false);
        compare(panel.attacks.attacks.length, 0);
        compare(JSON.stringify(available), original);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        keyClick(Qt.Key_Escape);
        tryCompare(menu, "visible", false);
        compare(addSpy.count, 1);
    }
    function test_slotChoices() {
        const snapshot = {attacks: [{id: "routine", name: "Routine", slots: [
            {id: "weapon", name: "Arma", selectionMode: AttacksPanel.ChooseOne, assignedGrantId: "sword",
                effectiveGrantIds: ["sword"], candidates: [
                    {grantId: "sword", name: "Spada <test>", accepted: true},
                    {grantId: "blocked", name: "Non consentito", accepted: false},
                    {grantId: "dagger", name: "Pugnale", accepted: true}]},
            {id: "empty", name: "Vuoto", selectionMode: AttacksPanel.ChooseOne, candidates: []},
            {id: "natural", name: "Naturali", selectionMode: AttacksPanel.AllMatching, candidates: []}
        ]}]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {attacks: snapshot, width: 260});
        assignSpy.target = panel;
        assignSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "routine_routine"));
        waitForRendering(panel);
        const button = findChild(panel, "choose_routine_weapon");
        mouseClick(button);
        const menu = button.choicesPopup;
        tryCompare(menu, "opened", true);
        compare(menu.itemAt(0).text, "Spada <test>");
        compare(menu.itemAt(0).contentItem.textFormat, Text.PlainText);
        compare(menu.itemAt(1).text, "Pugnale");
        verify(menu.itemAt(0).checked);
        mouseClick(menu.itemAt(1));
        compare(assignSpy.count, 1);
        compare(Array.from(assignSpy.signalArguments[0]), ["routine", "weapon", "dagger"]);
        tryCompare(menu, "visible", false);
        compare(JSON.stringify(snapshot), original);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        mouseClick(button);
        tryCompare(menu, "visible", false);
        compare(assignSpy.count, 1);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        keyClick(Qt.Key_Escape);
        tryCompare(menu, "visible", false);
        compare(assignSpy.count, 1);
        verify(!findChild(panel, "choose_routine_natural").enabled);
        mouseClick(findChild(panel, "choose_routine_empty"));
        const emptyMenu = findChild(panel, "choose_routine_empty").choicesPopup;
        tryCompare(emptyMenu, "opened", true);
        compare(emptyMenu.itemAt(0).text, "Nessun attacco selezionabile");
        verify(!emptyMenu.itemAt(0).enabled);
        emptyMenu.close();
    }
    function test_incompleteAndBlocked() {
        const panel = createTemporaryObject(component, testCase, {attacks: {attacks: [
            {id: "state", name: "Routine", actionId: "action", complete: false, usable: false}
        ]}, actions: {categories: [{actions: [{id: "action", usable: true}]}]}});
        waitForRendering(panel);
        const header = findChild(panel, "routine_state");
        verify(findChild(panel, "incomplete_state").visible);
        verify(header.enabled);
        mouseClick(header);
        verify(header.checked);
        panel.actions = {categories: [{actions: [{id: "action", usable: false,
            inhibitions: [{reason: "Blocco <test>"}, {reason: "Secondo motivo"}]}]}]};
        verify(!header.enabled);
        verify(!header.checked);
        verify(!findChild(panel, "slots_state").visible);
        const reason = findChild(panel, "blockedReason_state");
        verify(reason.visible);
        compare(reason.text, "Blocco <test>\nSecondo motivo");
        compare(reason.textFormat, Text.PlainText);
        mouseClick(header);
        verify(!header.checked);
        panel.actions = {categories: [{actions: [{id: "action", usable: true}]}]};
        verify(header.enabled);
        verify(!reason.visible);
        verify(!header.checked);
        mouseClick(header);
        verify(header.checked);
    }
    function test_calculatedAttacks() {
        const entries = [15, 10, 5].map(bonus => ({strikeName: "Spada", totalAttackBonusAdjustment: -99,
            strike: {attackAbilityOptions: [{attackBonus: bonus, criticalConfirmationBonus: bonus + 2}], criticalProfile: {threatMinimum: 19, multiplier: 2}},
            normalDamage: {components: [{occurrences: 1, component: {effectiveDice: {expression: "1d8"}}}], abilityOptions: [{bonus: 4}]},
            criticalDamage: {components: [
                {occurrences: 2, component: {effectiveDice: {expression: "1d8"}}},
                {occurrences: 1, component: {effectiveDice: {expression: "1d6"}}}
            ], abilityOptions: [{bonus: 9}]}}));
        const snapshot = {attacks: [{id: "full", name: "Completo", slots: [], calculatedAttacks: entries}]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {attacks: snapshot, width: 260});
        waitForRendering(panel);
        verify(!findChild(panel, "attack_full_0").visible);
        mouseClick(findChild(panel, "routine_full"));
        for (let i = 0; i < 3; ++i) {
            verify(findChild(panel, "attack_full_" + i).visible);
            compare(findChild(panel, "attackBonus_full_" + i).text, "+" + [15, 10, 5][i]);
            compare(findChild(panel, "confirmation_full_" + i).text, "Conferma critico: +" + [17, 12, 7][i]);
            verify(findChild(panel, "confirmation_full_" + i).visible);
            compare(findChild(panel, "criticalDamage_full_" + i).text, "Danni critici: 2 × (1d8) + 1d6 +9");
        }
        verify(findChild(panel, "attack_full_3") === null);
        const details = findChild(panel, "attackDetails_full_0");
        verify(!details.visible);
        const info = findChild(panel, "routineInfo_full");
        waitForRendering(panel);
        mouseClick(info);
        verify(details.visible);
        verify(details.summaryText.includes("Tiro per colpire"));
        verify(details.summaryText.includes("+17"));
        mouseClick(info);
        verify(!details.visible);
        compare(panel.damageSummary(entries[0].normalDamage), "1d8 +4");
        compare(panel.criticalSummary(entries[0]), "19–20 / ×2");
        compare(panel.attackBonus({strike: {attackAbilityOptions: [{attackBonus: -2}, {attackBonus: 3}]}}), "-2 / +3");
        compare(panel.damageSummary(null), "—");
        compare(panel.confirmationBonus({}), "—");
        compare(panel.confirmationBonus({strike: {attackAbilityOptions: [{attackBonus: 4}]}}), "—");
        compare(panel.confirmationBonus({strike: {attackAbilityOptions: [{criticalConfirmationBonus: 0}, {criticalConfirmationBonus: -2}]}}), "+0 / -2");
        compare(JSON.stringify(snapshot), original);
        panel.attacks = {attacks: [{id: "full", name: "Completo", calculatedAttacks: [
            {strike: {attackAbilityOptions: [{attackBonus: 15, criticalConfirmationBonus: 15}]}}
        ]}]};
        findChild(panel, "routine_full").checked = true;
        verify(!findChild(panel, "confirmation_full_0").visible);
        verify(!panel.hasDifferentConfirmation({}));
        verify(!panel.hasDifferentConfirmation({strike: {attackAbilityOptions: [{attackBonus: 4}]}}));
        verify(panel.hasDifferentConfirmation({strike: {attackAbilityOptions: [
            {attackBonus: 4, criticalConfirmationBonus: 4}, {attackBonus: 2, criticalConfirmationBonus: 3}
        ]}}));
        verify(panel.hasDifferentConfirmation({strike: {attackAbilityOptions: [
            {attackBonus: 4, criticalConfirmationBonus: 2}, {attackBonus: 2, criticalConfirmationBonus: 4}
        ]}}));
    }
    function test_actionCosts() {
        const panel = createTemporaryObject(component, testCase, {
            attacks: {attacks: Array.from({length: 8}, (_, i) => ({id: String(i), name: "Routine", actionId: "action" + i}))},
            actions: {categories: [{actions: Array.from({length: 8}, (_, i) => ({id: "action" + i, baseCost: 0, effectiveCost: i}))}]}
        });
        waitForRendering(panel);
        for (let i = 0; i < 8; ++i) {
            const icon = findChild(panel, "cost_" + i);
            compare(icon.cost, i);
            verify(icon.description.length > 0);
            verify(icon.x + icon.width <= findChild(panel, "routine_" + i).width);
        }
        panel.actions = null;
        compare(findChild(panel, "cost_0").cost, -1);
        compare(findChild(panel, "cost_0").description, "Costo non disponibile");
    }
    function test_routinesAndAdd() {
        const panel = createTemporaryObject(component, testCase, {attacks: {attacks: [
            {id: "first", name: "Spada <test>", calculatedAttacks: [{strikeName: "Not displayed"}]},
            {id: "second", name: "Arco"}
        ]}});
        addSpy.target = panel;
        addSpy.clear();
        waitForRendering(panel);
        compare(findChild(panel, "routine_first").text, "Spada <test>");
        compare(findChild(panel, "routine_first").contentItem.textFormat, Text.PlainText);
        verify(findChild(panel, "routine_second").mapToItem(panel, 0, 0).y > findChild(panel, "routine_first").mapToItem(panel, 0, 0).y);
        verify(findChild(panel, "attackSummary") === null);
        mouseClick(findChild(panel, "addRoutine"));
        compare(addSpy.count, 0);
        const emptyMenu = findChild(panel, "addRoutine").routinesPopup;
        tryCompare(emptyMenu, "opened", true);
        compare(emptyMenu.itemAt(0).text, "Nessuna routine disponibile");
        emptyMenu.close();
        compare(panel.attacks.attacks.length, 2);
        panel.width = 260;
        waitForRendering(panel);
        compare(findChild(panel, "routine_first").width, 260);
        panel.attacks = null;
        verify(findChild(panel, "addRoutine").enabled);
    }
    function test_slotExpansion() {
        const snapshot = {attacks: [{id: "test", name: "Routine", slots: [
            {id: "weapon", name: "Arma", selectionMode: AttacksPanel.ChooseOne, effectiveGrantIds: ["sword"],
                candidates: [{grantId: "sword", name: "Spada <test>"}]},
            {id: "empty", name: "Secondaria", selectionMode: AttacksPanel.ChooseOne, effectiveGrantIds: []},
            {id: "natural", name: "Naturali", selectionMode: AttacksPanel.AllMatching, effectiveGrantIds: ["bite", "claw"],
                candidates: [{grantId: "bite", name: "Morso"}, {grantId: "claw", name: "Artiglio"}]}
        ]}]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {attacks: snapshot, width: 260});
        waitForRendering(panel);
        const header = findChild(panel, "routine_test");
        const slots = findChild(panel, "slots_test");
        verify(!slots.visible);
        mouseClick(header);
        verify(slots.visible);
        compare(slots.width, 260);
        compare(findChild(panel, "selection_test_weapon").text, "Spada <test>");
        compare(findChild(panel, "selection_test_weapon").textFormat, Text.PlainText);
        compare(findChild(panel, "selection_test_empty").text, "Nessun attacco assegnato");
        compare(findChild(panel, "selection_test_natural").text, "Morso\nArtiglio");
        mouseClick(header);
        verify(!slots.visible);
        header.forceActiveFocus();
        keyClick(Qt.Key_Space);
        verify(slots.visible);
        compare(JSON.stringify(snapshot), original);
        compare(panel.slotSelection({assignedGrantId: "missing"}), "Assegnazione non disponibile");
        compare(panel.slotSelection({selectionMode: AttacksPanel.AllMatching}), "Nessun attacco compatibile");
    }
}
