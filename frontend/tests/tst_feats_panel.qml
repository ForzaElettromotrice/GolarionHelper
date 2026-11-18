import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "FeatsPanel"
    width: 500; height: 900
    visible: true
    when: windowShown
    Component { id: component; FeatsPanel {} }
    SignalSpy { id: selectSpy; signalName: "selectRequested" }
    SignalSpy { id: extraSpy; signalName: "extraRequested" }
    SignalSpy { id: removeSpy; signalName: "removeRequested" }
    SignalSpy { id: removeExtraSpy; signalName: "removeExtraSlotRequested" }
    function fixture() {
        return {slots: [
            {id: "normal", source: "Classe", origin: FeatsPanel.Source, available: false, featId: "focus"},
            {id: "empty", source: "Livello 3", origin: FeatsPanel.Source, available: true, allowedType: FeatsPanel.Combat},
            {id: "extra", source: "Extra", origin: FeatsPanel.Extra, available: true, featId: "bonus"}
        ], feats: [
            {slotId: "normal", id: "focus", name: "Talento <test>", description: "Test <description>", active: false, types: [FeatsPanel.Combat],
                choices: [{prompt: "Arma", selectedOptions: [{name: "Spada <test>"}]}]},
            {slotId: "extra", id: "bonus", name: "Extra", active: true, types: [], choices: []}
        ]};
    }
    function test_displayAndDetails() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {feats: snapshot, width: 260});
        waitForRendering(panel);
        compare(findChild(panel, "featName_normal").text, "Talento <test>");
        verify(findChild(panel, "featStatus_normal").visible);
        verify(!findChild(panel, "featStatus_extra").visible);
        verify(findChild(panel, "selectFeat_empty").visible);
        const select = findChild(panel, "selectFeat_empty");
        const source = findChild(panel, "emptyFeatSource_empty");
        compare(source.text, "Livello 3");
        verify(source.mapToItem(select, 0, 0).y >= 0);
        verify(source.mapToItem(select, source.width, source.height).y <= select.height);
        verify(!findChild(panel, "featName_empty").visible);
        verify(!findChild(panel, "selectFeat_normal").visible);
        const details = findChild(panel, "featDetails_normal");
        mouseClick(findChild(panel, "featInfo_normal"));
        verify(details.visible);
        verify(details.text.includes("&lt;description&gt;"));
        verify(details.summaryText.includes("Spada <test>"));
        verify(details.summaryText.includes("slot non è più disponibile"));
        compare(details.width, 260);
        mouseClick(findChild(panel, "featInfo_normal"));
        verify(!details.visible);
        compare(JSON.stringify(snapshot), original);
        compare(panel.restrictions({allowedType: FeatsPanel.Combat}), "Combattimento");
        compare(panel.restrictions({allowedFeatIds: ["one"]}), "Scelta da un insieme specifico");
    }
    function test_commandsAndOwnership() {
        const snapshot = fixture();
        const panel = createTemporaryObject(component, testCase, {feats: snapshot});
        selectSpy.target = panel; extraSpy.target = panel; removeSpy.target = panel; removeExtraSpy.target = panel;
        selectSpy.clear(); extraSpy.clear(); removeSpy.clear(); removeExtraSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "selectFeat_empty"));
        compare(selectSpy.count, 1);
        compare(Array.from(selectSpy.signalArguments[0]), ["empty"]);
        mouseClick(findChild(panel, "addExtraFeat"));
        compare(extraSpy.count, 1);
        mouseClick(findChild(panel, "removeFeat_normal"));
        compare(Array.from(removeSpy.signalArguments[0]), ["normal"]);
        mouseClick(findChild(panel, "removeFeat_extra"));
        compare(Array.from(removeExtraSpy.signalArguments[0]), ["extra"]);
        compare(snapshot.slots.length, 3);
        compare(snapshot.feats.length, 2);
        panel.feats = {slots: [{id: "unavailable", source: "Assente", available: false}], feats: []};
        verify(!findChild(panel, "selectFeat_unavailable").enabled);
        verify(findChild(panel, "addExtraFeat").enabled);
        panel.feats = null;
        verify(findChild(panel, "addExtraFeat").enabled);
    }
}
