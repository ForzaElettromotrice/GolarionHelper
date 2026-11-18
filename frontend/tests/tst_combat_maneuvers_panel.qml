import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "CombatManeuversPanel"
    width: 500; height: 900
    visible: true
    when: windowShown
    Component { id: component; CombatManeuversPanel {} }
    function test_valuesAndDetails() {
        const snapshot = {baseAttackBonus: 6, maneuvers: [{type: CombatManeuversPanel.Disarm,
            bonus: {abilityOptions: [
                {source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, totalValue: 17},
                {source: "Alternativa <test>", abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 2, totalValue: 13}
            ], modifiers: {modifiers: [
                {active: true, type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, resolvedValue: 2, source: "Talento"},
                {active: false, source: "Nascosto"}]}},
            defense: {strengthModifier: 4, dexterityModifier: 2, appliedDexterityModifier: 0, totalValue: 21,
                dexterityBonusSuppressed: true, dexteritySuppressions: [{source: "Condizione"}], modifiers: {modifiers: []}}
        }]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {combatManeuvers: snapshot});
        waitForRendering(panel);
        compare(panel.nameFor(CombatManeuversPanel.DirtyTrick), "Sporco trucco");
        compare(findChild(panel, "bmc_2").text, "+17 / +13");
        compare(findChild(panel, "dmc_2").text, "21");
        const details = findChild(panel, "maneuverDetails_2");
        verify(!details.visible);
        mouseClick(findChild(panel, "maneuverInfo_2"));
        verify(details.visible);
        verify(details.summaryText.includes("+6 BAB"));
        verify(details.summaryText.includes("+4 Forza"));
        verify(details.summaryText.includes("+0 Destrezza"));
        verify(details.summaryText.includes("+2 Talento"));
        verify(!details.summaryText.includes("Nascosto"));
        verify(details.summaryText.includes("Condizione"));
        verify(details.text.includes("&lt;test&gt;"));
        compare(details.width, panel.width - 28);
        mouseClick(findChild(panel, "maneuverInfo_2"));
        verify(!details.visible);
        compare(JSON.stringify(snapshot), original);
    }
    function test_missingAndNarrow() {
        const panel = createTemporaryObject(component, testCase, {combatManeuvers: null, width: 280});
        waitForRendering(panel);
        for (let i = 0; i < 10; ++i) {
            compare(findChild(panel, "bmc_" + i).text, "—");
            compare(findChild(panel, "dmc_" + i).text, "—");
            verify(findChild(panel, "maneuverName_" + i).text.length > 0);
        }
        const info = findChild(panel, "maneuverInfo_0");
        verify(info.mapToItem(panel, info.width, 0).x <= panel.width);
        mouseClick(info);
        verify(findChild(panel, "maneuverDetails_0").summaryText.includes("Dati non disponibili"));
        panel.combatManeuvers = {baseAttackBonus: 0, maneuvers: [{type: CombatManeuversPanel.BullRush,
            bonus: {abilityOptions: [{totalValue: -1}, {totalValue: -1}]}, defense: {totalValue: 9}}]};
        compare(findChild(panel, "bmc_0").text, "-1");
        compare(findChild(panel, "dmc_0").text, "9");
    }
}
