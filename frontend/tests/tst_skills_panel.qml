import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "SkillsPanel"
    width: 800
    height: 600
    visible: true
    when: windowShown
    MouseArea { x: 650; y: 450; width: 100; height: 100; onClicked: testCase.forceActiveFocus() }
    Component { id: component; SkillsPanel {} }
    property var panel
    SignalSpy { id: requested; signalName: "specializationAddRequested" }
    SignalSpy { id: rankRequested; signalName: "ranksChangeRequested" }
    SignalSpy { id: removalRequested; signalName: "specializationRemoveRequested" }
    SignalSpy { id: classSkillRequested; signalName: "classSkillChangeRequested" }
    function sample() {
        return [
            { type: 34, name: "Volare", resourceName: "skill.fly", ranks: 0, classSkill: false, usable: true, abilityOptions: [{totalValue: -2}] },
            { name: "Acrobazia", resourceName: "skill.acrobatics", ranks: 2, effectiveRanks: 6, classSkill: true, usable: true, abilityOptions: [{totalValue: 7}] },
            { type: SkillsPanel.Craft, specializationId: "glass", name: "Vetro", resourceName: "skill.craft.glass", ranks: 1, classSkill: false, usable: true, abilityOptions: [{totalValue: 3}] }
        ];
    }
    function test_customClassSkillWaitsForSnapshot() {
        classSkillRequested.target = panel;
        classSkillRequested.clear();
        const checkbox = findChild(findChild(panel, "skill.fly"), "classSkill");
        mouseClick(checkbox);
        compare(classSkillRequested.count, 1);
        compare(classSkillRequested.signalArguments[0][1], true);
        verify(!checkbox.checked);
        panel.skills = sample().map(entry => entry.resourceName === "skill.fly"
            ? Object.assign({}, entry, {classSkill: true, customClassSkill: true}) : entry);
        const updated = findChild(findChild(panel, "skill.fly"), "classSkill");
        verify(updated.checked);
        mouseClick(updated);
        compare(classSkillRequested.count, 2);
        compare(classSkillRequested.signalArguments[1][1], false);
        verify(updated.checked);
    }
    function test_categoryClassSkillUsesAuthoritativeOwnership() {
        classSkillRequested.target = panel;
        classSkillRequested.clear();
        const categoryName = "specializationCategory" + SkillsPanel.Craft;
        const row = findChild(panel, categoryName);
        verify(row !== null);
        const checkbox = findChild(row, "classSkill");
        mouseClick(checkbox);
        compare(classSkillRequested.count, 1);
        compare(Array.from(classSkillRequested.signalArguments[0]), [SkillsPanel.Craft, true]);
        panel.skillCategories = [{type: SkillsPanel.Craft, classSkill: true, customClassSkill: false}];
        const granted = findChild(findChild(panel, categoryName), "classSkill");
        verify(granted.checked);
        verify(!granted.checkable);
    }
    function init() {
        panel = createTemporaryObject(component, testCase, { skills: sample() });
        verify(panel !== null);
        waitForRendering(panel);
        requested.target = panel;
        requested.clear();
        rankRequested.target = panel;
        rankRequested.clear();
        removalRequested.target = panel;
        removalRequested.clear();
        mouseMove(testCase, 700, 500);
    }
    function test_orderAndValues() {
        compare(panel.sortedSkills[0].name, "Acrobazia");
        compare(panel.skills[0].name, "Volare");
        const row = findChild(panel, "skill.acrobatics");
        compare(findChild(row, "score").text, "+7");
        compare(findChild(row, "effectiveRanks").text, "6");
        const checkbox = findChild(row, "classSkill");
        verify(checkbox.checked);
        mouseClick(checkbox);
        verify(checkbox.checked);
        compare(findChild(findChild(panel, "skill.fly"), "score").text, "-2");
    }
    function test_situationalBonusesAreVisibleWithoutOpeningDetails() {
        const snapshot = sample();
        snapshot[1].abilityOptions[0].modifiers = {conditionalTotals: [
            {condition: "Per saltare", value: 2}, {condition: "Su superfici scivolose", value: -2},
            {condition: "In condizioni normali", value: 0}
        ]};
        snapshot[1].abilityOptions.push({totalValue: 5, modifiers: {conditionalTotals: [{condition: "Per saltare", value: 1}]}});
        panel.skills = snapshot;
        const row = findChild(panel, "skill.acrobatics");
        const list = findChild(row, "situationalBonuses");
        const cards = Array.from(list.children).filter(child => child.objectName === "situationalBonus");
        compare(cards.length, 2);
        compare(findChild(cards[0], "situationalBonusValue").text, "+2 / +1");
        compare(findChild(cards[1], "situationalBonusValue").text, "-2 / —");
        verify(list.visible);
        const score = findChild(row, "score");
        compare(list.x + list.width, score.background.mapToItem(row, score.background.width, 0).x);
        verify(!findChild(row, "skillDetails").visible);
        compare(findChild(row, "score").text, "+7 / +5");
    }
    function test_updatesAndAlternativeScores() {
        const next = sample();
        next[0].usable = false;
        next[1].abilityOptions = [{totalValue: 7}, {totalValue: 9}, {totalValue: 7}];
        panel.skills = next;
        compare(findChild(findChild(panel, "skill.fly"), "score").text, "—");
        compare(findChild(findChild(panel, "skill.acrobatics"), "score").text, "+7 / +9");
    }
    function test_narrowAndEmpty() {
        panel.setExpanded(SkillsPanel.Craft, true);
        panel.width = 320;
        waitForRendering(panel);
        const row = findChild(panel, "skill.craft.glass");
        verify(row.width <= panel.width);
        verify(findChild(row, "skillName").height <= row.height);
        panel.skills = [];
        compare(panel.sortedSkills.length, 3);
    }
    function test_specializationInputs() {
        for (const type of [SkillsPanel.Craft, SkillsPanel.Perform, SkillsPanel.Profession]) {
            const row = findChild(panel, "specializationCategory" + type);
            const input = findChild(row, "specializationInput");
            verify(input.visible);
            input.forceActiveFocus();
            input.text = "  ";
            keyClick(Qt.Key_Return);
            compare(requested.count, 0);
            input.forceActiveFocus();
            input.text = "  Alchimia  ";
            keyClick(Qt.Key_Return);
            compare(requested.count, 1);
            compare(requested.signalArguments[0][0], type);
            compare(requested.signalArguments[0][1], "Alchimia");
            compare(panel.skills.length, 3);
            requested.clear();
        }
    }
    function test_specializedNamesAndCategoryDeduplication() {
        panel.skills = [
            {type: SkillsPanel.Craft, name: "Artigianato", resourceName: "skill.craft"},
            {type: SkillsPanel.Craft, specializationId: "alchemy", name: "Alchimia", resourceName: "skill.craft.alchemy", ranks: 0, usable: true, classSkill: false, abilityOptions: []}
        ];
        compare(panel.sortedSkills.length, 4);
        compare(findChild(findChild(panel, "skill.craft.alchemy"), "skillName").text, "Artigianato (Alchimia)");
        panel.width = 320;
        waitForRendering(panel);
        const row = findChild(panel, "specializationCategory2");
        const input = findChild(row, "specializationInput");
        verify(input.mapToItem(row, input.width, 0).x <= row.width);
    }
    function test_expandCollapseAndUpdates() {
        const category = findChild(panel, "specializationCategory2");
        const toggle = findChild(category, "specializationToggle");
        const child = findChild(panel, "skill.craft.glass");
        verify(!child.visible);
        const initialHeight = panel.height;
        mouseClick(toggle);
        tryCompare(child, "visible", true);
        waitForRendering(panel);
        verify(panel.height > initialHeight);
        const input = findChild(category, "specializationInput");
        input.text = "Bozza";
        mouseClick(toggle);
        verify(!child.visible);
        compare(input.text, "Bozza");
        mouseClick(toggle);
        const next = sample();
        next.push({type: SkillsPanel.Craft, specializationId: "alchemy", name: "Alchimia", resourceName: "skill.craft.alchemy", ranks: 0, classSkill: false, usable: true, abilityOptions: []});
        panel.skills = next;
        verify(findChild(panel, "skill.craft.alchemy").visible);
        const names = panel.sortedSkills.filter(entry => entry.specializationId).map(entry => entry.name);
        compare(names[0], "Artigianato (Alchimia)");
        compare(names[1], "Artigianato (Vetro)");
        verify(!panel.expandedTypes.includes(SkillsPanel.Perform));
        compare(requested.count, 0);
    }
    function test_classCheckboxOnCategoryOnly() {
        panel.skills = [
            {type: SkillsPanel.Craft, name: "Artigianato", resourceName: "skill.craft", classSkill: true},
            {type: SkillsPanel.Craft, specializationId: "alchemy", name: "Alchimia", resourceName: "skill.craft.alchemy", classSkill: true, ranks: 0, usable: true, abilityOptions: []}
        ];
        panel.setExpanded(SkillsPanel.Craft, true);
        const parentCheckbox = findChild(findChild(panel, "specializationCategory2"), "classSkill");
        verify(parentCheckbox.visible);
        verify(parentCheckbox.checked);
        verify(!findChild(findChild(panel, "skill.craft.alchemy"), "classSkill").visible);
        mouseClick(parentCheckbox);
        verify(parentCheckbox.checked);
        const next = panel.skills.slice();
        next[0] = Object.assign({}, next[0], {classSkill: false});
        panel.skills = next;
        // An individually granted specialization must not mark the whole category.
        verify(!findChild(findChild(panel, "specializationCategory2"), "classSkill").checked);
    }
    function test_rankEditingAndIndentation() {
        const row = findChild(panel, "skill.acrobatics");
        const input = findChild(row, "ranks");
        verify(!input.visible);
        mouseMove(findChild(row, "ranksCell"), 20, 20);
        tryCompare(input, "visible", true);
        compare(input.text, "2");
        mouseClick(input);
        input.selectAll();
        keyClick(Qt.Key_5);
        keyClick(Qt.Key_Return);
        compare(rankRequested.count, 1);
        verify(!input.activeFocus);
        compare(rankRequested.signalArguments[0][0], "skill.acrobatics");
        compare(rankRequested.signalArguments[0][1], 5);
        compare(panel.skills[1].ranks, 2);
        compare(findChild(row, "score").text, "+7");
        panel.setExpanded(SkillsPanel.Craft, true);
        const specialization = findChild(panel, "skill.craft.glass");
        waitForRendering(panel);
        compare(findChild(specialization, "skillName").leftPadding, 20);
        const specializedInput = findChild(specialization, "ranks");
        mouseMove(findChild(specialization, "ranksCell"), 20, 20);
        mouseClick(specializedInput);
        verify(specializedInput.activeFocus);
        specializedInput.text = "0";
        mouseMove(testCase, 700, 500);
        verify(specializedInput.activeFocus);
        verify(specializedInput.visible);
        compare(rankRequested.count, 1);
        mouseClick(testCase, 700, 500);
        compare(rankRequested.count, 2);
        compare(rankRequested.signalArguments[1][0], "skill.craft.glass");
        compare(rankRequested.signalArguments[1][1], 0);
        verify(findChild(row, "effectiveRanks").visible);
        compare(findChild(row, "effectiveRanks").text, "6");
    }
    function test_invalidRanksAndCancel() {
        const input = findChild(findChild(panel, "skill.acrobatics"), "ranks");
        mouseMove(findChild(findChild(panel, "skill.acrobatics"), "ranksCell"), 20, 20);
        for (const value of ["", "-1", "2.5", "abc", "2147483648"]) {
            input.forceActiveFocus();
            input.text = value;
            keyClick(Qt.Key_Return);
            compare(input.text, "2");
        }
        input.forceActiveFocus();
        input.text = "8";
        keyClick(Qt.Key_Escape);
        compare(input.text, "2");
        compare(rankRequested.count, 0);
    }
    function test_pinsAndCustomRemoval() {
        const next = sample();
        next.push({type: SkillsPanel.Craft, specializationId: "custom", resourceName: "skill.craft.custom", name: "Personalizzata", custom: true, ranks: 0, effectiveRanks: 0, classSkill: false, usable: true, abilityOptions: []});
        panel.skills = next;
        panel.setExpanded(SkillsPanel.Craft, true);
        waitForRendering(panel);
        const canonical = findChild(panel, "skill.craft.glass");
        const custom = findChild(panel, "skill.craft.custom");
        verify(!findChild(canonical, "removeSpecialization").visible);
        verify(findChild(custom, "removeSpecialization").visible);
        const pin = findChild(canonical, "pinSpecialization");
        mouseClick(pin);
        verify(pin.checked);
        panel.setExpanded(SkillsPanel.Craft, false);
        verify(canonical.visible);
        verify(!custom.visible);
        waitForRendering(panel);
        mouseClick(pin);
        verify(!canonical.visible);
        panel.setExpanded(SkillsPanel.Craft, true);
        waitForRendering(panel);
        mouseClick(findChild(custom, "pinSpecialization"));
        panel.skills = next.slice();
        panel.setExpanded(SkillsPanel.Craft, false);
        const updatedCustom = findChild(panel, "skill.craft.custom");
        verify(updatedCustom.visible);
        waitForRendering(panel);
        mouseClick(findChild(updatedCustom, "removeSpecialization"));
        compare(removalRequested.count, 1);
        compare(removalRequested.signalArguments[0][0], SkillsPanel.Craft);
        compare(removalRequested.signalArguments[0][1], "custom");
        compare(panel.skills.length, 4);
        // Only the host accepts a deletion; the component emits a request.
        panel.skills = sample();
        verify(findChild(panel, "skill.craft.custom") === null);
    }
    function test_detailsAndBudget() {
        const next = sample();
        Object.assign(next[1], {classSkillBonus: 3, appliedArmorCheckPenalty: 2,
            rankGrants: [{source: "Oggetto", ranks: 6}],
            abilityOptions: [{abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 2, source: "Base", totalValue: 11,
                modifiers: {total: 2, modifiers: [
                    {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Cintura", resolvedValue: 2, active: true},
                    {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Nascosto", resolvedValue: 4, active: false}
                ]}}, {abilityType: AbilityScoresPanel.Intelligence, abilityModifier: 4, source: "Alternativa", totalValue: 13, modifiers: {total: 2, modifiers: []}}]});
        panel.skills = next;
        panel.spentRanks = 3;
        panel.maxRanks = 10;
        compare(findChild(panel, "rankBudget").text, "Gradi spesi / disponibili: 3 / 10");
        const row = findChild(panel, "skill.acrobatics");
        const details = findChild(row, "skillDetails");
        verify(!details.visible);
        waitForRendering(panel);
        const initialHeight = panel.height;
        mouseClick(findChild(row, "skillInfo"));
        verify(details.visible);
        verify(details.text.includes('<b><font size="4">Gradi</font></b>'));
        verify(details.text.includes('<b><font size="4">Punteggio</font></b>'));
        compare(details.escapeText("Fonte <b> & bonus"), "Fonte &lt;b&gt; &amp; bonus");
        verify(details.summaryText.startsWith("Gradi\n  +2 Assegnati da te\n  +6 Oggetto"));
        verify(details.summaryText.includes("\nPunteggio\n"));
        verify(details.summaryText.includes("+2 Destrezza"));
        verify(details.summaryText.includes("Intelligenza — Alternativa"));
        verify(details.summaryText.includes("+4 Intelligenza"));
        verify(details.summaryText.includes("+3 Abilità di classe"));
        verify(details.summaryText.includes("-2 Armatura"));
        verify(details.summaryText.includes("Totale: 6"));
        verify(details.summaryText.includes("Totale: +11"));
        verify(!details.summaryText.includes("Dettaglio —"));
        verify(details.summaryText.includes("Bonus di potenziamento\n    +2 Cintura"));
        verify(!details.summaryText.includes("Nascosto"));
        waitForRendering(panel);
        verify(panel.height > initialHeight);
        panel.width = 320;
        waitForRendering(panel);
        verify(details.width <= panel.width);
        mouseClick(findChild(row, "skillInfo"));
        verify(!details.visible);
        compare(rankRequested.count, 0);
    }
}
