import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "HitPointsPanel"
    width: 500; height: 1000
    visible: true
    when: windowShown
    Component { id: component; HitPointsPanel {} }
    property var panel
    SignalSpy { id: damageSpy; signalName: "damageRequested" }
    SignalSpy { id: healSpy; signalName: "healRequested" }
    SignalSpy { id: addSpy; signalName: "temporaryAddRequested" }
    SignalSpy { id: removeSpy; signalName: "temporaryRemoveRequested" }
    function init() {
        panel = createTemporaryObject(component, testCase, {hitPoints: {
            baseMax: 0, max: 30, current: -2, nonLethal: 4,
            maxContributions: {contributions: [{id: "Classe", resolvedValue: 30}]},
            temporary: {total: 5, pools: [{id: "spell.test", remaining: 5}]}
        }});
        for (const spy of [damageSpy, healSpy, addSpy, removeSpy]) { spy.target = panel; spy.clear(); }
        waitForRendering(panel);
    }
    function test_snapshotAndDamage() {
        compare(findChild(panel, "currentAndMaximum").text, "-2 / 30");
        const input = findChild(panel, "amount");
        mouseClick(findChild(panel, "currentAndMaximum"));
        input.text = "-6";
        keyClick(Qt.Key_Return);
        compare(Array.from(damageSpy.signalArguments[0]), [6, HitPointsPanel.Lethal]);
        mouseClick(findChild(panel, "currentAndMaximum"));
        waitForRendering(panel);
        mouseClick(findChild(panel, "nonLethal"));
        input.forceActiveFocus();
        input.text = "-6";
        keyClick(Qt.Key_Return);
        compare(Array.from(damageSpy.signalArguments[1]), [6, HitPointsPanel.NonLethal]);
        mouseClick(findChild(panel, "currentAndMaximum"));
        input.text = "+6";
        keyClick(Qt.Key_Return);
        compare(Array.from(healSpy.signalArguments[0]), [6]);
        compare(panel.hitPoints.current, -2);
        compare(input.text, "");
        compare(findChild(panel, "temporaryTotal").text, "+5");
        mouseClick(findChild(panel, "currentAndMaximum"));
        for (const command of ["6", "+0", "--6", "-1.5", "+2147483648"]) {
            input.text = command;
            keyClick(Qt.Key_Return);
        }
        compare(damageSpy.count, 2);
        compare(healSpy.count, 1);
        keyClick(Qt.Key_Escape);
        verify(!panel.editing);
        mouseClick(findChild(panel, "currentAndMaximum"));
        input.text = "-9";
        input.focus = false;
        tryCompare(panel, "editing", false);
        compare(damageSpy.count, 2);
        for (const text of ["-1", "0", "1.5", "2147483648", "abc"])
            verify(!panel.positiveAmount(text));
    }
    function test_temporaryAndDetails() {
        mouseClick(findChild(panel, "addTemporary"));
        waitForRendering(panel);
        findChild(panel, "temporarySource").text = " spell.new ";
        findChild(panel, "temporaryAmount").text = "8";
        mouseClick(findChild(panel, "confirmTemporary"));
        compare(Array.from(addSpy.signalArguments[0]), ["spell.new", 8]);
        mouseClick(findChild(panel, "maximumInfo"));
        waitForRendering(panel);
        mouseClick(findChild(panel, "removeTemporary"));
        compare(Array.from(removeSpy.signalArguments[0]), ["spell.test"]);
        compare(panel.hitPoints.temporary.total, 5);
        compare(panel.maximumRows().filter(row => row.kind === InfoDetails.Total)[0].text, "Totale: 30");
        panel.width = 320;
        waitForRendering(panel);
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(findChild(panel, "temporaryPopup"), "opened", true);
        verify(findChild(panel, "confirmTemporary").mapToItem(panel, findChild(panel, "confirmTemporary").width, 0).x <= panel.width);
    }
    function test_rightClickSelectsNonLethal() {
        const display = findChild(panel, "currentAndMaximum");
        const input = findChild(panel, "amount");
        const checkbox = findChild(panel, "nonLethal");
        mouseClick(display, display.width / 2, display.height / 2, Qt.RightButton);
        verify(panel.editing);
        verify(checkbox.checked);
        verify(input.activeFocus);
        input.text = "-4";
        keyClick(Qt.Key_Return);
        compare(Array.from(damageSpy.signalArguments[0]), [4, HitPointsPanel.NonLethal]);
        mouseClick(display);
        verify(!checkbox.checked);
        keyClick(Qt.Key_Escape);
        compare(damageSpy.count, 1);
    }
    function test_outsideClickCancelsExceptCheckbox() {
        mouseClick(findChild(panel, "currentAndMaximum"));
        const input = findChild(panel, "amount");
        input.text = "-7";
        waitForRendering(panel);
        mouseClick(findChild(panel, "nonLethal"));
        waitForRendering(panel);
        verify(panel.editing);
        compare(input.text, "-7");
        mouseClick(testCase, 450, 800);
        tryCompare(panel, "editing", false);
        compare(input.text, "");
        compare(damageSpy.count, 0);
        compare(healSpy.count, 0);
        mouseClick(findChild(panel, "currentAndMaximum"));
        mouseClick(findChild(panel, "maximumInfo"));
        tryCompare(panel, "editing", false);
        verify(findChild(panel, "maximumInfo").checked);
    }
    function test_proportionalBars() {
        const caption = findChild(panel, "hitPointCaption");
        const value = findChild(panel, "currentAndMaximum");
        const temporary = findChild(panel, "temporaryExtension");
        compare(caption.height, value.height);
        compare(temporary.height, value.height);
        compare(caption.mapToItem(panel, 0, 0).y, value.mapToItem(panel, 0, 0).y);
        compare(temporary.mapToItem(panel, 0, 0).y, value.mapToItem(panel, 0, 0).y);
        compare(temporary.mapToItem(panel, temporary.width, 0).x, findChild(panel, "hitPointDetailsFrame").width);
        panel.hitPoints = {max: 40, current: 30, temporary: {total: 20}};
        compare(panel.hitPointFraction, 0.75);
        compare(panel.temporaryWidth, 54);
        panel.hitPoints = {max: 40, current: -4, temporary: {total: 1}};
        compare(panel.hitPointFraction, 0);
        compare(panel.temporaryWidth, 54);
        panel.hitPoints = {max: 0, current: 0, temporary: {total: 0}};
        compare(panel.hitPointFraction, 0);
        compare(panel.temporaryWidth, 0);
        verify(!findChild(panel, "temporaryExtension").visible);
        waitForRendering(panel);
        compare(value.mapToItem(panel, value.width, 0).x, findChild(panel, "hitPointDetailsFrame").width);
    }
    function test_sharedInfoAndTemporaryAddVisibility() {
        const add = findChild(panel, "addTemporary");
        mouseMove(testCase, 450, 800);
        compare(add.opacity, 1);
        const extension = findChild(panel, "temporaryExtension");
        mouseMove(extension, extension.width / 2, extension.height / 2);
        compare(add.opacity, 1);
        mouseClick(findChild(panel, "maximumInfo"));
        verify(findChild(panel, "maximumDetails").visible);
        verify(findChild(panel, "temporaryDetails").visible);
        mouseClick(findChild(panel, "maximumInfo"));
        verify(!findChild(panel, "maximumDetails").visible);
        verify(!findChild(panel, "temporaryDetails").visible);
        panel.hitPoints = {max: 30, current: 20, temporary: {total: 0, pools: []}};
        mouseMove(testCase, 450, 800);
        compare(add.opacity, 1);
    }
    function test_missing() {
        panel.hitPoints = null;
        compare(findChild(panel, "currentAndMaximum").text, "— / —");
        findChild(panel, "amount").text = "3";
        verify(!findChild(panel, "amount").enabled);
    }
    function test_baseMaximumIsNotDisplayed() {
        panel.hitPoints = {baseMax: 10, max: 40, current: 40,
            maxContributions: {contributions: [{id: "Classe", resolvedValue: 30}]},
            temporary: {total: 0, pools: []}};
        const rows = panel.maximumRows();
        compare(rows.length, 3);
        compare(rows[1].text, "+30 Classe");
        compare(rows[2].text, "Totale: 40");
    }
    function test_temporaryPopupDoesNotMoveDetails() {
        mouseClick(findChild(panel, "maximumInfo"));
        waitForRendering(panel);
        const previousHeight = panel.height;
        const detailsY = findChild(panel, "temporaryDetails").y;
        const popup = findChild(panel, "temporaryPopup");
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(popup, "opened", true);
        compare(panel.height, previousHeight);
        compare(findChild(panel, "temporaryDetails").y, detailsY);
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(popup, "visible", false);
        compare(addSpy.count, 0);
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(popup, "opened", true);
        findChild(panel, "temporarySource").text = "Fonte";
        findChild(panel, "temporaryAmount").text = "8";
        keyClick(Qt.Key_Escape);
        tryCompare(popup, "opened", false);
        compare(addSpy.count, 0);
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(popup, "opened", true);
        compare(findChild(panel, "temporarySource").text, "");
        mouseClick(testCase, 450, 800);
        tryCompare(popup, "opened", false);
        compare(addSpy.count, 0);
        mouseClick(findChild(panel, "addTemporary"));
        tryCompare(popup, "opened", true);
        findChild(panel, "temporarySource").text = "Fonte";
        const amount = findChild(panel, "temporaryAmount");
        amount.text = "8";
        amount.forceActiveFocus();
        keyClick(Qt.Key_Return);
        tryCompare(popup, "opened", false);
        compare(Array.from(addSpy.signalArguments[0]), ["Fonte", 8]);
    }
}
