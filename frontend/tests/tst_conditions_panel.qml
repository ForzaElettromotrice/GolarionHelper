import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "ConditionsPanel"
    when: windowShown
    width: 600
    height: 420
    visible: true
    Component { id: panelComponent; ConditionsPanel {} }
    SignalSpy { id: addSpy; signalName: "addConditionRequested" }
    SignalSpy { id: removeSpy; signalName: "removeConditionsRequested" }

    function test_togglesOnlyManualEntries() {
        const prone = {id: "prone", name: "Prono", effectiveSeverity: 0, entries: []};
        const unconscious = {id: "unconscious", name: "Privo di sensi", effectiveSeverity: 1,
            entries: [{id: "hp", origin: ConditionsPanel.SourceControlled}]};
        const dead = {id: "dead", name: "Morto", effectiveSeverity: 0, entries: []};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 600,
            conditions: {conditions: [prone, unconscious, dead]}, automaticConditionIds: ["dead"]});
        addSpy.target = removeSpy.target = panel;
        const tile = id => findChild(panel, "conditionTile_" + id);
        verify(tile("dead").visible && !tile("dead").enabled);
        verify(tile("unconscious").applied && !tile("unconscious").enabled);
        tile("prone").click();
        compare(addSpy.signalArguments[0][0], {conditionId: "prone", severity: 1, parameter: null});
        verify(!tile("prone").applied);
        panel.conditions = {conditions: [Object.assign({}, prone, {effectiveSeverity: 1, entries: [
            {id: "ability", origin: ConditionsPanel.SourceControlled}, {id: "manual-prone", origin: ConditionsPanel.Manual}
        ]}), unconscious, dead]};
        verify(tile("prone").applied && tile("prone").enabled);
        tile("prone").click();
        compare(removeSpy.signalArguments[0][0], ["manual-prone"]);
        verify(tile("prone").applied);
    }
}
