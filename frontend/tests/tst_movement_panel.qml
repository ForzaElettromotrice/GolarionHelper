import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "MovementPanel"
    width: 500; height: 1000
    visible: true
    when: windowShown
    Component { id: component; MovementPanel {} }
    function test_valuesAndDetails() {
        const grants = [
            {id: "flight", type: MovementPanel.Fly, source: "Volo <test>", baseUnits: 12, effectiveUnits: 10, usable: true, maneuverability: MovementPanel.Good, modifiers: {modifiers: []}},
            {id: "land", type: MovementPanel.Land, source: "Razza", baseUnits: 6, effectiveUnits: 5, usable: true, modifiers: {modifiers: []},
                run: {supported: true, usable: true, effectiveMultiplier: 4, distanceUnits: 20}}
        ];
        const panel = createTemporaryObject(component, testCase, {movement: {grants: grants}});
        waitForRendering(panel);
        compare(panel.grants[0].id, "land");
        compare(grants[0].id, "flight");
        const land = findChild(panel, "movement_land");
        compare(findChild(land, "movementSpeed").text, "7,5 m");
        compare(findChild(land, "runDistance").text, "30 m");
        mouseClick(findChild(land, "movementInfo"));
        const detail = findChild(land, "movementDetails");
        verify(detail.text.includes("30 m (×4)"));
        verify(detail.text.includes("Velocità: 7,5 m"));
        const flight = findChild(panel, "movement_flight");
        verify(findChild(flight, "runDistance").visible);
        compare(findChild(flight, "runDistance").text, "—");
        compare(findChild(flight, "movementSpeed").x, findChild(land, "movementSpeed").x);
        mouseClick(findChild(flight, "movementInfo"));
        verify(findChild(flight, "movementDetails").text.includes("Volo &lt;test&gt;"));
        verify(findChild(flight, "movementDetails").text.includes("Manovrabilità: Buona"));
        panel.width = 260;
        waitForRendering(panel);
        verify(detail.width <= panel.width);
        verify(flight.y >= land.y + land.height);
    }
    function test_blockedAndEmpty() {
        const panel = createTemporaryObject(component, testCase, {movement: {grants: [
            {id: "blocked", type: 0, source: "Base", baseUnits: 6, effectiveUnits: 0, usable: false,
                modifiers: {modifiers: []}, run: {supported: true, usable: false, notUsableReasons: ["Condizione di esempio"]}}
        ]}});
        compare(findChild(findChild(panel, "movement_blocked"), "movementSpeed").text, "—");
        compare(findChild(findChild(panel, "movement_blocked"), "runDistance").text, "—");
        verify(panel.detailRows(panel.grants[0]).some(row => row.text === "Condizione di esempio"));
        panel.movement = null;
        compare(panel.grants.length, 0);
    }
}
