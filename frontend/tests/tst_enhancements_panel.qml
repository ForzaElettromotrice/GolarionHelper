import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "EnhancementsPanel"
    when: windowShown
    Component { id: panelComponent; EnhancementsPanel {} }
    SignalSpy { id: activationSpy; signalName: "setEnhancementGroupActiveRequested" }

    function test_requestsActivationAndWaitsForSnapshot() {
        const snapshot = {groups: [{id: "bless", name: "Benedizione", active: false}]};
        const panel = createTemporaryObject(panelComponent, testCase, {enhancements: snapshot, width: 450});
        activationSpy.target = panel;
        activationSpy.clear();
        const toggle = () => findChild(panel, "enhancementToggle_bless");
        verify(!toggle().checked);
        toggle().click();
        compare(activationSpy.count, 1);
        compare(activationSpy.signalArguments[0][0], "bless");
        compare(activationSpy.signalArguments[0][1], true);
        verify(!toggle().checked);
        panel.enhancements = {groups: [{id: "bless", name: "Benedizione", active: true}]};
        verify(toggle().checked);
        toggle().click();
        compare(activationSpy.signalArguments[1][1], false);
        verify(toggle().checked);
        verify(!snapshot.groups[0].active);
    }

}
