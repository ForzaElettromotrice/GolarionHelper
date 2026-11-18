import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "SavingThrowsPanel"
    width: 700
    height: 1000
    visible: true
    when: windowShown
    Component { id: component; SavingThrowsPanel {} }
    property var panel
    function sample() {
        return [2, 0, 1].map(type => ({type: type, baseValue: 2,
            baseContributions: {total: 2, contributions: [{id: "class.test", resolvedValue: 2}]},
            abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Constitution, abilityModifier: -1, totalValue: 4}],
            modifiers: {total: 3, modifiers: [
                {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Resistance, source: "Mantello <b>", active: true, resolvedValue: 3},
                {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Resistance, source: "Inattivo", active: false, resolvedValue: 5}
            ]}}));
    }
    function init() {
        panel = createTemporaryObject(component, testCase, {savingThrows: sample()});
        verify(panel !== null);
        waitForRendering(panel);
    }
    function test_orderAndAuthoritativeScores() {
        compare(panel.nameFor(SavingThrowsPanel.Fortitude), "Tempra");
        compare(panel.nameFor(SavingThrowsPanel.Reflex), "Riflessi");
        compare(panel.nameFor(SavingThrowsPanel.Will), "Volontà");
        for (let i = 0; i < 3; ++i) {
            const row = findChild(panel, "savingThrow" + i);
            compare(findChild(row, "score").text, "+4");
            verify(findChild(row, "ranks") === null);
            if (i) verify(row.y > findChild(panel, "savingThrow" + (i - 1)).y);
        }
        const next = sample();
        next[1].abilityOptions.push({source: "Alternativa", abilityType: AbilityScoresPanel.Charisma, abilityModifier: 5, totalValue: 10});
        panel.savingThrows = next;
        compare(findChild(findChild(panel, "savingThrow0"), "score").text, "+4 / +10");
    }
    function test_detailAndNarrowLayout() {
        const row = findChild(panel, "savingThrow0");
        const details = findChild(row, "savingThrowDetails");
        verify(!details.visible);
        mouseClick(findChild(row, "savingThrowInfo"));
        verify(details.visible);
        verify(details.text.includes("+2 class.test"));
        verify(details.text.includes("-1 Costituzione"));
        verify(details.text.includes('<font size="3" color="#666666"><i>Bonus di resistenza</i></font>'));
        verify(details.text.includes('<b><font size="4">Punteggio</font></b>'));
        verify(details.text.includes('&nbsp;&nbsp;&nbsp;&nbsp;+3 Mantello &lt;b&gt;'));
        verify(details.text.includes("+3 Mantello &lt;b&gt;"));
        verify(!details.text.includes("Inattivo"));
        verify(details.text.includes("Totale: +4"));
        verify(!details.text.includes("Totale base"));
        verify(!details.text.includes("+2 Base"));
        compare(details.rows.filter(line => line.kind === InfoDetails.Heading).length, 1);
        compare(details.rows.filter(line => line.kind === InfoDetails.Total).length, 1);
        compare(details.rows.filter(line => line.text === "+2 class.test").length, 1);
        panel.width = 320;
        waitForRendering(panel);
        verify(details.width <= panel.width);
        verify(findChild(panel, "savingThrow1").y >= row.y + row.height);
        mouseClick(findChild(row, "savingThrowInfo"));
        verify(!details.visible);
    }
    function test_missingData() {
        panel.savingThrows = [];
        compare(findChild(findChild(panel, "savingThrow0"), "score").text, "—");
    }
}
