import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "InitiativePanel"
    width: 500; height: 900
    visible: true
    when: windowShown
    Component { id: component; InitiativePanel {} }
    function test_valuesAndDetails() {
        const panel = createTemporaryObject(component, testCase, {initiative: {
            modifiers: {total: 99, modifiers: []},
            abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 3, totalValue: 7,
                modifiers: {total: 4, modifiers: [
                    {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, source: "Talento <test>", active: true, resolvedValue: 4},
                    {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, source: "Inattivo", active: false, resolvedValue: 9}
                ]}}]}});
        waitForRendering(panel);
        compare(findChild(panel, "initiativeScore").text, "+7");
        const details = findChild(panel, "initiativeDetails");
        verify(!details.visible);
        mouseClick(findChild(panel, "initiativeInfo"));
        verify(details.visible);
        verify(details.text.includes("+3 Destrezza"));
        verify(details.text.includes("+4 Talento &lt;test&gt;"));
        verify(!details.text.includes("Inattivo"));
        verify(details.text.includes("Totale: +7"));
        panel.width = 240;
        waitForRendering(panel);
        verify(details.width <= panel.width);
        verify(details.y >= findChild(panel, "initiativeScore").height);
    }
    function test_alternativesAndMissing() {
        const panel = createTemporaryObject(component, testCase, {initiative: {abilityOptions: [
            {abilityType: 1, abilityModifier: -2, totalValue: -2, modifiers: {total: 0, modifiers: []}},
            {abilityType: 5, abilityModifier: 4, totalValue: 4, modifiers: {total: 0, modifiers: []}}
        ]}});
        compare(findChild(panel, "initiativeScore").text, "-2 / +4");
        panel.initiative = null;
        compare(findChild(panel, "initiativeScore").text, "—");
        compare(panel.detailRows[0].text, "Dati non disponibili.");
    }
}
