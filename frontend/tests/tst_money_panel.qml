import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "MoneyPanel"
    width: 600; height: 650
    visible: true
    when: windowShown
    Component { id: panelComponent; MoneyPanel {} }
    SignalSpy { id: addSpy; signalName: "addMoneyRequested" }
    SignalSpy { id: conversionSpy; signalName: "convertMoneyRequested" }

    function test_convertBetweenContainers() {
        const snapshot = {total: {gold: 13, silver: 9}, containers: [{containerId: "home", amounts: {gold: 13, silver: 9}}]};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 570, money: snapshot,
            containers: [{id: "worn", name: "Indossati"}, {id: "home", name: "Casa"}]});
        conversionSpy.target = panel; conversionSpy.clear();
        waitForRendering(panel);
        const source = findChild(panel, "moneySource");
        const destination = findChild(panel, "moneyDestination");
        const change = findChild(panel, "moneyConvert_gold_silver");
        const reverse = findChild(panel, "moneyConvert_silver_gold");
        const silver = findChild(panel, "moneyCard_silver");
        const gold = findChild(panel, "moneyCard_gold");
        compare(silver.y, gold.y);
        compare(reverse.x, change.x);
        verify(reverse.y < change.y);
        const midpoint = change.mapToItem(panel, change.width / 2, 0).x;
        fuzzyCompare(midpoint, (silver.x + silver.width + gold.x) / 2, 1);
        verify(!change.enabled);
        source.currentIndex = 1;
        compare(destination.currentValue, "home");
        destination.currentIndex = 0;
        verify(change.enabled);
        verify(!reverse.enabled);
        mouseClick(change);
        compare(Array.from(conversionSpy.signalArguments[0]), [MoneyPanel.Gold, MoneyPanel.Silver, 1, 10, "home", "worn"]);
        compare(snapshot.total.gold, 13);
    }

    function test_addMoneyRequest() {
        const snapshot = {total: {gold: 13}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 500, money: snapshot,
            containers: [{id: "home", name: "Casa"}, {id: "worn", name: "Indossati"}]});
        addSpy.target = panel; addSpy.clear();
        waitForRendering(panel);
        const destination = findChild(panel, "moneyDestination");
        compare(destination.currentValue, "worn");
        const total = findChild(panel, "moneyAmount_gold");
        destination.currentIndex = 0;
        mouseClick(total);
        const input = findChild(panel, "moneyAddQuantity_gold");
        tryCompare(input, "activeFocus", true);
        compare(input.text, "13");
        input.text = "0";
        input.accepted();
        compare(addSpy.count, 0);
        input.text = "30";
        input.accepted();
        compare(Array.from(addSpy.signalArguments[0]), [MoneyPanel.Gold, 17, "home"]);
        verify(!input.visible);
        compare(findChild(panel, "moneyAmount_gold").text, "13");
    }

    function test_authoritativeTotals() {
        const panel = createTemporaryObject(panelComponent, testCase, {width: 500,
            money: {total: {copper: 1200, silver: 0, gold: 13, platinum: 2}, containers: [{amounts: {gold: 99}}]}});
        waitForRendering(panel);
        compare(findChild(panel, "moneyAmount_copper").text, (1200).toLocaleString(Qt.locale("it_IT"), "f", 0));
        compare(findChild(panel, "moneyAmount_silver").text, "0");
        compare(findChild(panel, "moneyAmount_gold").text, "13");
        compare(findChild(panel, "moneyAmount_platinum").text, "2");
        panel.money = null;
        compare(findChild(panel, "moneyAmount_gold").text, "—");
    }
}
