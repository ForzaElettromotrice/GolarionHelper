import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "InventoryPanel"
    width: 1100; height: 1300
    visible: true
    when: windowShown
    Component { id: component; InventoryPanel {} }
    SignalSpy { id: containerSpy; signalName: "addContainerRequested" }
    function test_addRootContainer() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        containerSpy.target = panel; containerSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "addInventoryContainer"));
        tryCompare(panel.containerPopup, "opened", true);
        const name = findChild(panel.containerPopup.contentItem, "inventoryContainerName");
        const carried = findChild(panel.containerPopup.contentItem, "inventoryContainerCarriedWeight");
        const effects = findChild(panel.containerPopup.contentItem, "inventoryContainerPossessionEffects");
        compare(carried.checked, false);
        compare(effects.checked, true);
        name.text = "   ";
        name.accepted();
        compare(containerSpy.count, 0);
        verify(panel.containerPopup.opened);
        name.text = "  Deposito  ";
        carried.checked = true;
        effects.checked = false;
        name.accepted();
        compare(containerSpy.count, 1);
        const request = containerSpy.signalArguments[0][0];
        compare(request.name, "Deposito");
        compare(request.contributesToCarriedWeight, true);
        compare(request.allowsPossessionEffects, false);
        tryCompare(panel.containerPopup, "opened", false);
        compare(JSON.stringify(snapshot), original);
        mouseClick(findChild(panel, "addInventoryContainer"));
        tryCompare(panel.containerPopup, "opened", true);
        compare(name.text, "");
        compare(carried.checked, false);
        compare(effects.checked, true);
        panel.containerPopup.reject();
        compare(containerSpy.count, 1);
    }
    function fixture() {
        return {totalCarriedWeightGrams: 9876, equipmentSlots: [{slot: 1, capacity: 1, itemIds: ["armor"]}], items: [
            {id: "armor", name: "Armatura <test>", quantity: 1, effectiveWeightGrams: 5000, equipped: true},
            {id: "bag", name: "Zaino", quantity: 1, effectiveWeightGrams: 2000, ownedContainerId: "bag.container"},
            {id: "pouch", name: "Borsa", quantity: 1, effectiveWeightGrams: 1000, ownedContainerId: "pouch.container"},
            {id: "potion", name: "Pozione", quantity: 2, effectiveWeightGrams: 500}
        ], containers: [
            {id: "worn", name: "Indossati", itemIds: ["armor", "bag"]},
            {id: "bag.container", name: "Zaino", ownerItemId: "bag", itemIds: ["pouch"]},
            {id: "pouch.container", name: "Borsa", ownerItemId: "pouch", itemIds: ["potion"]},
            {id: "home", name: "Casa", itemIds: []}
        ]};
    }
    function test_hierarchyAndSlots() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        compare(panel.rows().map(row => row.id), ["worn", "armor", "bag", "pouch", "potion", "home"]);
        compare(panel.rows().find(row => row.id === "potion").depth, 2);
        const armorName = findChild(panel, "inventoryName_armor");
        const bagName = findChild(panel, "inventoryName_bag");
        const pouchName = findChild(panel, "inventoryName_pouch");
        const potionName = findChild(panel, "inventoryName_potion");
        compare(armorName.mapToItem(panel, 0, 0).x, bagName.mapToItem(panel, 0, 0).x);
        compare(pouchName.mapToItem(panel, 0, 0).x - bagName.mapToItem(panel, 0, 0).x, 8);
        compare(potionName.mapToItem(panel, 0, 0).x - pouchName.mapToItem(panel, 0, 0).x, 8);
        const bag = findChild(panel, "inventoryRow_bag");
        const pouch = findChild(panel, "inventoryRow_pouch");
        const potion = findChild(panel, "inventoryRow_potion");
        const pouchPosition = pouch.mapToItem(bag, 0, 0);
        verify(pouchPosition.x > 0 && pouchPosition.y > 0);
        verify(pouchPosition.y + pouch.height < bag.height);
        verify(pouch.width < bag.width);
        verify(potion.mapToItem(pouch, 0, 0).y + potion.height < pouch.height);
        compare(panel.slot(1).itemIds[0], "armor");
        verify(findChild(panel, "equipmentSlot_13") !== null);
        compare(findChild(panel, "carriedWeight").text, "Peso trasportato: " + panel.weight(9876));
        mouseClick(findChild(panel, "toggleContainer_bag"));
        verify(findChild(panel, "inventoryRow_potion") === null);
        verify(findChild(panel, "inventoryRow_bag") !== null);
        waitForRendering(panel);
        mouseClick(findChild(panel, "toggleContainer_bag"));
        waitForRendering(panel);
        tryVerify(() => findChild(panel, "inventoryRow_potion") !== null);
        mouseClick(findChild(panel, "toggleContainer_worn"));
        compare(panel.rows().map(row => row.id), ["worn", "home"]);
        compare(JSON.stringify(snapshot), original);
    }
    function test_responsiveAndEmpty() {
        const panel = createTemporaryObject(component, testCase, {inventory: fixture(), width: 1000});
        waitForRendering(panel);
        const equipment = findChild(panel, "equipmentDiagram");
        verify(equipment.width < 600);
        const silhouette = findChild(panel, "equipmentSilhouette");
        tryCompare(silhouette, "status", Image.Ready);
        verify(panel.slotAnchor(InventoryPanel.Head).y < panel.slotAnchor(InventoryPanel.Neck).y);
        verify(panel.slotAnchor(InventoryPanel.Neck).y < panel.slotAnchor(InventoryPanel.Feet).y);
        for (const type of panel.slotOrder) {
            const point = panel.slotAnchor(type);
            verify(point.x >= silhouette.x && point.x <= silhouette.x + silhouette.width);
            verify(point.y >= silhouette.y && point.y <= silhouette.y + silhouette.height);
        }
        panel.width = 340;
        waitForRendering(panel);
        compare(equipment.width, 340);
        verify(findChild(panel, "equipmentSlot_13").x >= 0);
        panel.inventory = null;
        compare(panel.rows().length, 0);
    }
    function test_itemDetailsToggle() {
        const snapshot = fixture();
        snapshot.items[0].description = "Descrizione <test>";
        snapshot.items[0].usagePools = [{id: "charges", remaining: 2, capacity: 5}];
        snapshot.items[0].choices = [{prompt: "Scelta", options: [{name: "Selezionata", selected: true}, {name: "Nascosta", selected: false}]}];
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        const details = findChild(panel, "inventoryDetails_armor");
        const row = findChild(panel, "inventoryRow_armor");
        const initialHeight = row.height;
        const name = findChild(panel, "inventoryName_armor");
        const info = findChild(panel, "inventoryInfo_armor");
        fuzzyCompare(info.x, name.x + name.width + 4, 1);
        fuzzyCompare(info.y + info.height / 2, name.y + name.height / 2, 1);
        const nameX = name.x;
        mouseMove(testCase, 1090, 1290);
        tryCompare(info, "opacity", 0);
        mouseMove(row, row.width - 2, 2);
        tryCompare(info, "opacity", 1);
        compare(name.x, nameX);
        verify(!details.visible);
        mouseClick(findChild(panel, "inventoryInfo_armor"));
        waitForRendering(panel);
        verify(details.visible);
        verify(details.text.includes("&lt;test&gt;"));
        verify(details.summaryText.includes("2 / 5"));
        verify(details.summaryText.includes("Selezionata"));
        verify(!details.summaryText.includes("Nascosta"));
        verify(row.height > initialHeight);
        compare(details.width, row.width - 16);
        verify(!findChild(panel, "inventoryInfo_worn").visible);
        mouseMove(testCase, 1090, 1290);
        tryCompare(info, "opacity", 0);
        verify(details.visible);
        mouseMove(info, info.width / 2, info.height / 2);
        tryCompare(info, "opacity", 1);
        mouseClick(findChild(panel, "inventoryInfo_armor"));
        verify(!details.visible);
        compare(JSON.stringify(snapshot), original);
    }
    function test_nestedHoverOnlyShowsOwnInfo() {
        const panel = createTemporaryObject(component, testCase, {inventory: fixture(), width: 1000});
        waitForRendering(panel);
        const bagInfo = findChild(panel, "inventoryInfo_bag");
        const pouchInfo = findChild(panel, "inventoryInfo_pouch");
        const potionInfo = findChild(panel, "inventoryInfo_potion");
        mouseMove(findChild(panel, "inventoryName_bag"), 2, 2);
        tryCompare(bagInfo, "opacity", 1);
        tryCompare(pouchInfo, "opacity", 0);
        tryCompare(potionInfo, "opacity", 0);
        mouseMove(findChild(panel, "inventoryName_pouch"), 2, 2);
        tryCompare(bagInfo, "opacity", 0);
        tryCompare(pouchInfo, "opacity", 1);
        tryCompare(potionInfo, "opacity", 0);
        mouseMove(findChild(panel, "inventoryName_potion"), 2, 2);
        tryCompare(bagInfo, "opacity", 0);
        tryCompare(pouchInfo, "opacity", 0);
        tryCompare(potionInfo, "opacity", 1);
        mouseMove(findChild(panel, "inventoryName_bag"), 2, 2);
        tryCompare(bagInfo, "opacity", 1);
        tryCompare(pouchInfo, "opacity", 0);
        tryCompare(potionInfo, "opacity", 0);
    }
    function test_weightsAndContainerCapacity() {
        const snapshot = fixture();
        snapshot.items[3].intrinsicWeightGrams = 500;
        snapshot.containers[1].currentContentsWeightGrams = 1000;
        snapshot.containers[1].maximumContentsWeightGrams = 5000;
        snapshot.containers[2].currentContentsWeightGrams = 500;
        snapshot.containers[2].maximumContentsWeightGrams = 0;
        snapshot.containers[0].currentContentsWeightGrams = 7000;
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        compare(findChild(panel, "inventoryWeight_bag").text, panel.weight(1000) + " / " + panel.weight(5000));
        compare(findChild(panel, "inventoryWeight_pouch").text, panel.weight(500) + " / " + panel.weight(0));
        compare(findChild(panel, "inventoryWeight_worn").text, panel.weight(7000) + " / ∞");
        compare(findChild(panel, "inventoryWeight_potion").text, panel.weight(500));
        verify(panel.itemInfoRows("potion").some(row => row.text === "Peso unitario: " + panel.weight(250)));
        verify(!panel.itemInfoRows("potion").some(row => row.text.startsWith("Peso complessivo:")));
    }
}
