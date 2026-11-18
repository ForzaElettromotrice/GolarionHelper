import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "AbilityScoresPanel"
    width: 800
    height: 600
    visible: true
    when: windowShown
    MouseArea { x: 650; y: 450; width: 100; height: 100; onClicked: testCase.forceActiveFocus() }
    Component { id: panelComponent; AbilityScoresPanel {} }
    property var panel
    SignalSpy { id: requested; signalName: "baseValueChangeRequested" }
    function sample() {
        return [18, 13, 14, 10, 12, 10].map((score, type) => ({
            type: type, baseValue: score - 2, totalValue: score, modifier: Math.floor((score - 10) / 2)
        }));
    }
    function init() {
        panel = createTemporaryObject(panelComponent, testCase, { abilities: sample() });
        verify(panel !== null);
        waitForRendering(panel);
        requested.target = panel;
        requested.clear();
        mouseMove(testCase, 700, 500);
    }
    function test_sixRowsAndThreeColumns() {
        for (let i = 0; i < 6; ++i) {
            const row = findChild(panel, "ability" + i);
            verify(row !== null);
            compare(row.children.length, 4);
            compare(findChild(row, "totalValue").text, String(sample()[i].totalValue));
            compare(findChild(row, "modifier").text, String(sample()[i].modifier));
            verify(!findChild(row, "baseInput").visible);
            if (i > 0) verify(row.mapToItem(panel, 0, 0).y > findChild(panel, "ability" + (i - 1)).mapToItem(panel, 0, 0).y);
        }
    }
    function test_authoritativeValuesAndEnumOrder() {
        const next = sample().reverse();
        next[5].totalValue = 7;
        next[5].modifier = -4;
        panel.abilities = next;
        const strength = findChild(panel, "ability0");
        compare(findChild(strength, "totalValue").text, "7");
        compare(findChild(strength, "modifier").text, "-4");
    }
    function test_missingValues() {
        panel.abilities = [];
        compare(findChild(findChild(panel, "ability0"), "totalValue").text, "—");
    }
    function test_hoverEditAndExit() {
        const row = findChild(panel, "ability0");
        const input = findChild(row, "baseInput");
        const total = findChild(row, "totalValue");
        mouseMove(findChild(row, "scoreCell"), 30, 20);
        tryCompare(input, "visible", true);
        compare(input.text, "16");
        verify(!total.visible);
        mouseClick(input);
        input.selectAll();
        keyClick(Qt.Key_2);
        keyClick(Qt.Key_0);
        mouseMove(testCase, 700, 500);
        verify(input.activeFocus);
        verify(input.visible);
        compare(input.text, "20");
        compare(requested.count, 0);
        mouseMove(findChild(row, "scoreCell"), 30, 20);
        compare(input.text, "20");
        mouseMove(testCase, 700, 500);
        mouseClick(testCase, 700, 500);
        tryCompare(total, "visible", true);
        compare(requested.count, 1);
        compare(requested.signalArguments[0][0], AbilityScoresPanel.Strength);
        compare(requested.signalArguments[0][1], 20);
        compare(total.text, "18");
        compare(panel.abilities[0].baseValue, 16);
    }
    function test_enterEscapeAndInvalidValues() {
        const row = findChild(panel, "ability0");
        const input = findChild(row, "baseInput");
        mouseMove(findChild(row, "scoreCell"), 30, 20);
        tryCompare(input, "visible", true);
        mouseClick(input);
        input.text = "22";
        keyClick(Qt.Key_Return);
        compare(requested.count, 1);
        verify(!input.activeFocus);
        input.forceActiveFocus();
        input.text = "25";
        keyClick(Qt.Key_Escape);
        compare(input.text, "16");
        for (const value of ["", "0", "-1", "2.5", "2147483648", "abc"]) {
            input.text = value;
            findChild(row, "scoreCell").submit();
        }
        mouseMove(testCase, 700, 500);
        compare(requested.count, 1);
    }
    function test_externalBaseChangeDoesNotGetOverwritten() {
        const row = findChild(panel, "ability0");
        mouseMove(findChild(row, "scoreCell"), 30, 20);
        const input = findChild(row, "baseInput");
        tryCompare(input, "visible", true);
        input.text = "20";
        const next = sample();
        next[0].baseValue = 17;
        panel.abilities = next;
        mouseMove(testCase, 700, 500);
        compare(requested.count, 0);
    }
    function test_compactLayout() {
        for (const width of [320, 740]) {
            panel.width = width;
            waitForRendering(panel);
            const row = findChild(panel, "ability0");
            verify(row.width <= 340);
            verify(row.width <= panel.width);
            for (const cell of row.children) verify(cell.x + cell.width <= row.width + 1);
        }
    }
    function test_expandModifiersBelowRow() {
        const next = sample();
        next[0].modifiers = { total: 2, conditionalTotals: [], modifiers: [
            { type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Cintura", resolvedValue: 4, active: true },
            { type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Incantesimo", resolvedValue: 2, active: true },
            { type: AbilityScoresPanel.Penalty, source: "Affaticamento", resolvedValue: 2, active: true },
            { type: AbilityScoresPanel.Bonus, source: "Effetto sospeso", resolvedValue: 1, active: false }
        ] };
        panel.abilities = next;
        const row = findChild(panel, "ability0");
        const button = findChild(row, "detailsButton");
        const detail = findChild(row.parent, "modifierDetails");
        verify(!detail.visible);
        const initialHeight = panel.height;
        mouseClick(button);
        tryCompare(detail, "visible", true);
        verify(detail.summaryText.includes("Bonus di potenziamento\n    +4 Cintura\n    +2 Incantesimo"));
        verify(detail.summaryText.includes("Penalità\n    −2 Affaticamento"));
        verify(detail.text.includes('<font size="3" color="#666666"><i>Bonus di potenziamento</i></font>'));
        verify(!detail.text.includes("Effetto sospeso"));
        compare(detail.text.match(/Totale/g).length, 1);
        verify(detail.text.includes("Totale modificatori: +2"));
        waitForRendering(panel);
        verify(panel.height > initialHeight);
        const following = findChild(panel, "ability1");
        verify(following.mapToItem(panel, 0, 0).y >= detail.mapToItem(panel, 0, detail.height).y);
        mouseClick(button);
        tryCompare(detail, "visible", false);
        compare(requested.count, 0);
    }
}
