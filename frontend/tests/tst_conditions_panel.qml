import QtQuick
import QtTest
import Golarion.Sheet
import "../preview" as Preview

TestCase {
    id: testCase
    name: "ConditionsPanel"
    when: windowShown
    width: 600
    height: 420
    visible: true
    Component { id: panelComponent; ConditionsPanel {} }
    Component { id: previewComponent; Preview.Main {} }
    SignalSpy { id: addSpy; signalName: "addConditionRequested" }
    SignalSpy { id: removeSpy; signalName: "removeConditionsRequested" }

    function test_multipleBleedsCanBeRemovedIndividually() {
        const preview = createTemporaryObject(previewComponent, testCase, {conditionCatalog: [
            {id: "bleeding", name: "Sanguinante", entryParameterName: "Danno per turno"}
        ]});
        findChild(preview, "panelSelector").currentIndex = 20;
        const panel = findChild(preview, "conditionsPreview");
        waitForRendering(panel);
        const tile = findChild(panel, "conditionTile_bleeding");
        tile.click();
        const editor = findChild(panel, "bleedingEditor");
        tryCompare(editor, "opened", true);
        const damage = findChild(editor.contentItem, "bleedingDamage");
        const plus = findChild(editor.contentItem, "addBleeding");
        damage.text = "2d6 PF";
        plus.click();
        damage.text = "1 Forza";
        plus.click();
        const first = () => findChild(editor.contentItem, "removeBleeding_preview-manual-1");
        const second = () => findChild(editor.contentItem, "removeBleeding_preview-manual-2");
        tryVerify(() => first() && second());
        const reminders = findChild(preview, "remindersPreview");
        compare(reminders.reminders.messages, [
            "Sanguinante: 2d6 PF all’inizio del turno.",
            "Sanguinante: 1 Forza all’inizio del turno."
        ]);
        first().click();
        tryVerify(() => !first() && !!second());
        compare(reminders.reminders.messages, ["Sanguinante: 1 Forza all’inizio del turno."]);
        const updatedTile = findChild(panel, "conditionTile_bleeding");
        verify(updatedTile.applied);
        second().click();
        verify(!findChild(panel, "conditionTile_bleeding").applied);
        compare(reminders.reminders.messages, []);
        preview.close();
    }

    function test_negativeLevelsAndFearStepIndividually() {
        const preview = createTemporaryObject(previewComponent, testCase, {conditionCatalog: [
            {id: "negativeLevels", name: "Livelli Negativi", stages: [{severity: 1, name: "Livello Negativo"}]},
            {id: "fear", name: "Paura", stages: [{severity: 1, name: "Scosso"},
                {severity: 2, name: "Spaventato"}, {severity: 3, name: "In preda al panico"}]}
        ]});
        findChild(preview, "panelSelector").currentIndex = 20;
        const panel = findChild(preview, "conditionsPreview");
        waitForRendering(panel);
        const control = (prefix, id) => findChild(panel, prefix + "Condition_" + id);
        const value = id => findChild(panel, "conditionStep_" + id).text;
        const clickStep = (prefix, id) => {
            const button = control(prefix, id);
            waitForRendering(button);
            mouseClick(button);
        };
        verify(!control("decrease", "negativeLevels").enabled);
        clickStep("increase", "negativeLevels");
        clickStep("increase", "negativeLevels");
        compare(value("negativeLevels"), "2");
        clickStep("decrease", "negativeLevels");
        compare(value("negativeLevels"), "1");

        clickStep("increase", "fear");
        compare(value("fear"), "Scosso");
        clickStep("increase", "fear");
        compare(value("fear"), "Spaventato");
        clickStep("increase", "fear");
        compare(value("fear"), "In preda al panico");
        verify(!control("increase", "fear").enabled);
        clickStep("decrease", "fear");
        compare(value("fear"), "Spaventato");
        preview.close();
    }

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
