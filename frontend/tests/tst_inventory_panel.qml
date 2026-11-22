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
    SignalSpy { id: catalogItemSpy; signalName: "addCatalogItemRequested" }
    SignalSpy { id: customItemSpy; signalName: "addCustomItemRequested" }
    SignalSpy { id: unequipSpy; signalName: "unequipItemRequested" }
    function openContainerPicker(panel) {
        const add = findChild(panel, "addInventoryEntry");
        mouseClick(add);
        tryCompare(add.picker, "opened", true);
        mouseClick(findChild(add.picker.contentItem, "addInventoryContainer"));
    }
    function test_addItemMenuRequest() {
        const panel = createTemporaryObject(component, testCase, {inventory: fixture(), width: 1000});
        catalogItemSpy.target = panel; catalogItemSpy.clear();
        customItemSpy.target = panel; customItemSpy.clear();
        waitForRendering(panel);
        const add = findChild(panel, "addInventoryEntry");
        mouseClick(add);
        tryCompare(add.picker, "opened", true);
        const submenu = add.picker.menuAt(0);
        verify(submenu !== null);
        mouseClick(add.picker.itemAt(0));
        tryCompare(submenu, "opened", true);
        mouseClick(submenu.itemAt(0));
        compare(catalogItemSpy.count, 1);
        compare(customItemSpy.count, 0);
        tryCompare(add.picker, "opened", false);
        mouseClick(add);
        tryCompare(add.picker, "opened", true);
        mouseClick(add.picker.itemAt(0));
        tryCompare(submenu, "opened", true);
        mouseClick(submenu.itemAt(1));
        compare(catalogItemSpy.count, 1);
        compare(customItemSpy.count, 0);
        tryCompare(panel.customItemPopup, "opened", true);
        const name = findChild(panel.customItemPopup.contentItem, "customItemName");
        const weight = findChild(panel.customItemPopup.contentItem, "customItemWeight");
        panel.customItemPopup.submit();
        compare(customItemSpy.count, 0);
        verify(panel.customItemPopup.error.length > 0);
        name.text = "  Oggetto di prova  ";
        weight.text = "250";
        panel.customItemPopup.submit();
        compare(customItemSpy.count, 1);
        const definition = customItemSpy.signalArguments[0][0];
        compare(definition.name, "Oggetto di prova");
        compare(definition.weightGrams, 250);
        compare(definition.volumeMilliliters, 0);
        compare(definition.slot, null);
    }
    function test_dragEquippedItemToContainer() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        unequipSpy.target = panel; unequipSpy.clear();
        waitForRendering(panel);
        const source = findChild(panel, "equippedItem_armor");
        const target = findChild(panel, "inventoryDrop_bag");
        mousePress(source, 5, 5);
        mouseMove(source, 25, 5);
        mouseMove(target, 45, 12);
        mouseRelease(target, 45, 12);
        tryCompare(unequipSpy, "count", 1);
        compare(Array.from(unequipSpy.signalArguments[0]), ["armor", "bag.container"]);
        verify(panel.canRequestMove("armor", "worn", true));
        compare(JSON.stringify(snapshot), original);
    }
    SignalSpy { id: moveSpy; signalName: "moveItemRequested" }
    SignalSpy { id: reorderSpy; signalName: "reorderItemRequested" }
    SignalSpy { id: moveAndReorderSpy; signalName: "moveAndReorderItemRequested" }
    function test_dropBetweenItemsInAnotherContainer() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        moveAndReorderSpy.target = panel; moveAndReorderSpy.clear();
        waitForRendering(panel);
        verify(panel.reorderDestination("bag", "potion", false) === undefined);
        const source = findChild(panel, "inventoryName_potion");
        const target = findChild(panel, "inventoryReorderBefore_armor");
        mousePress(source, 5, 5);
        mouseMove(source, 25, 5);
        mouseMove(target, 45, 1);
        mouseRelease(target, 45, 1);
        tryCompare(moveAndReorderSpy, "count", 1);
        compare(Array.from(moveAndReorderSpy.signalArguments[0]), ["potion", "worn", "armor"]);
        compare(JSON.stringify(snapshot), original);
    }
    function test_dropNearEdgeReordersWithinContainer() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        reorderSpy.target = panel; reorderSpy.clear();
        waitForRendering(panel);
        verify(panel.reorderDestination("potion", "armor", false).moving);
        const source = findChild(panel, "inventoryName_bag");
        const target = findChild(panel, "inventoryReorderBefore_armor");
        mousePress(source, 5, 5);
        mouseMove(source, 25, 5);
        mouseMove(target, 45, 1);
        mouseRelease(target, 45, 1);
        tryCompare(reorderSpy, "count", 1);
        compare(Array.from(reorderSpy.signalArguments[0]), ["bag", "armor"]);
        compare(JSON.stringify(snapshot), original);
    }
    SignalSpy { id: quantitySpy; signalName: "itemQuantityChangeRequested" }
    function test_singleItemStackability() {
        const snapshot = fixture();
        snapshot.items[3].quantity = 1;
        snapshot.items[3].canStack = true;
        snapshot.items[1].canStack = false;
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        const potion = findChild(panel, "inventoryQuantity_potion");
        mouseMove(testCase, 1090, 1290);
        tryCompare(potion, "visible", false);
        mouseMove(findChild(panel, "inventoryName_potion"), 4, 4);
        tryCompare(potion, "visible", true);
        compare(potion.text, "×1");
        verify(!findChild(findChild(panel, "inventoryRow_potion"), "inventoryQuantityDecrease").enabled);
        mouseMove(findChild(panel, "inventoryName_bag"), 4, 4);
        verify(!findChild(panel, "inventoryQuantity_bag").visible);
    }
    function test_editStackQuantity() {
        const panel = createTemporaryObject(component, testCase, {inventory: fixture(), width: 1000});
        quantitySpy.target = panel; quantitySpy.clear();
        waitForRendering(panel);
        const quantity = findChild(panel, "inventoryQuantity_potion");
        compare(quantity.text, "×2");
        mouseClick(quantity);
        const input = findChild(findChild(panel, "inventoryRow_potion"), "inventoryQuantityInput");
        tryCompare(input, "activeFocus", true);
        compare(input.text, "2");
        input.text = "0";
        input.accepted();
        compare(quantitySpy.count, 0);
        input.text = "30";
        input.accepted();
        compare(Array.from(quantitySpy.signalArguments[0]), ["potion", 30]);
        compare(quantity.text, "×2");
        verify(!input.visible);
        mouseClick(quantity);
        tryCompare(input, "activeFocus", true);
        input.text = "8";
        keyClick(Qt.Key_Escape);
        verify(!input.visible);
        compare(quantitySpy.count, 1);
        mouseMove(quantity, quantity.width / 2, quantity.height / 2);
        const row = findChild(panel, "inventoryRow_potion");
        const increase = findChild(row, "inventoryQuantityIncrease");
        const decrease = findChild(row, "inventoryQuantityDecrease");
        tryCompare(increase, "visible", true);
        mouseClick(increase);
        compare(Array.from(quantitySpy.signalArguments[1]), ["potion", 3]);
        mouseClick(decrease);
        compare(Array.from(quantitySpy.signalArguments[2]), ["potion", 1]);
    }
    SignalSpy { id: mergeSpy; signalName: "mergeCoinStacksRequested" }
    SignalSpy { id: splitSpy; signalName: "splitCoinStackRequested" }
    function test_splitCoinStackFromInfo() {
        const snapshot = fixture();
        snapshot.items.push({id: "gold", itemDefinitionId: "goldCoin", name: "Oro", quantity: 5, containerId: "worn"});
        snapshot.containers[0].itemIds.push("gold");
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        splitSpy.target = panel; splitSpy.clear();
        waitForRendering(panel);
        const info = findChild(panel, "inventoryInfo_gold");
        const split = findChild(panel, "inventorySplit_gold");
        verify(!split.visible);
        mouseMove(info, 10, 10);
        mouseClick(info);
        tryCompare(split, "visible", true);
        waitForRendering(panel);
        mouseClick(split);
        tryCompare(split.editor, "opened", true);
        const quantity = findChild(split.editor.contentItem, "inventorySplitQuantity");
        compare(quantity.to, 4);
        quantity.value = 2;
        split.editor.submit();
        compare(Array.from(splitSpy.signalArguments[0]), ["gold", 2]);
        compare(snapshot.items[4].quantity, 5);
    }
    function test_dropCoinStackOnMatchingStack() {
        const snapshot = fixture();
        snapshot.items.push({id: "gold-a", itemDefinitionId: "goldCoin", name: "Oro", quantity: 5, containerId: "worn"});
        snapshot.items.push({id: "gold-b", itemDefinitionId: "goldCoin", name: "Oro", quantity: 8, containerId: "worn"});
        snapshot.items.push({id: "silver", itemDefinitionId: "silverCoin", name: "Argento", quantity: 3, containerId: "worn"});
        snapshot.containers[0].itemIds.push("gold-a", "gold-b", "silver");
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        mergeSpy.target = panel; mergeSpy.clear();
        waitForRendering(panel);
        verify(!panel.canMergeCoinStacks("gold-a", "silver"));
        verify(!panel.canMergeCoinStacks("gold-a", "gold-a"));
        const source = findChild(panel, "inventoryName_gold-a");
        const target = findChild(panel, "inventoryDrop_gold-b");
        mousePress(source, 5, 5);
        mouseMove(source, 25, 5);
        mouseMove(target, 45, 12);
        mouseRelease(target, 45, 12);
        tryCompare(mergeSpy, "count", 1);
        compare(Array.from(mergeSpy.signalArguments[0]), ["gold-a", "gold-b"]);
        compare(JSON.stringify(snapshot), original);
    }
    SignalSpy { id: equipSpy; signalName: "equipItemRequested" }
    function test_dropOnOccupiedEquipmentSlot() {
        const snapshot = fixture();
        snapshot.items[3].equipmentSlot = InventoryPanel.Armor;
        snapshot.items[3].quantity = 1;
        snapshot.items[3].containerId = "pouch.container";
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        equipSpy.target = panel;
        equipSpy.clear();
        waitForRendering(panel);
        const source = findChild(panel, "inventoryName_potion");
        const target = findChild(panel, "equipmentSlot_1");
        mousePress(source, 5, 5);
        mouseMove(source, 30, 5);
        mouseMove(target, 30, 12);
        mouseRelease(target, 30, 12);
        tryCompare(equipSpy, "count", 1);
        compare(Array.from(equipSpy.signalArguments[0]), ["potion", "armor", "pouch.container"]);
        compare(JSON.stringify(snapshot), original);
    }
    function test_dragItemToContainer() {
        const snapshot = fixture();
        snapshot.items[3].equipmentSlot = InventoryPanel.Ring;
        snapshot.items[3].quantity = 1;
        snapshot.equipmentSlots.push({slot: InventoryPanel.Ring, capacity: 2, itemIds: ["existing"]});
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        moveSpy.target = panel;
        moveSpy.clear();
        waitForRendering(panel);
        verify(!panel.canRequestMove("bag", "pouch.container"));
        verify(!panel.canRequestMove("bag", "bag.container"));
        verify(!panel.canRequestMove("armor", "bag.container"));
        verify(!panel.canRequestMove("potion", "pouch.container"));
        const source = findChild(panel, "inventoryName_potion");
        const target = findChild(panel, "inventoryDrop_bag");
        mousePress(source, 5, 5);
        mouseMove(source, 25, 5);
        mouseMove(target, 45, 16);
        tryCompare(findChild(panel, "equipmentSlot_0"), "compatible", true);
        compare(findChild(panel, "equipmentSlot_1").opacity, 0.38);
        verify(panel.canEquipInSlot(snapshot.items[3], {slot: InventoryPanel.Ring, capacity: 1, itemIds: ["existing"]}));
        verify(!panel.canEquipInSlot({equipmentSlot: InventoryPanel.Ring, quantity: 2}, snapshot.equipmentSlots[1]));
        mouseRelease(target, 45, 16);
        tryCompare(findChild(panel, "equipmentSlot_1"), "opacity", 1);
        tryCompare(moveSpy, "count", 1);
        compare(Array.from(moveSpy.signalArguments[0]), ["potion", "bag.container"]);
        compare(JSON.stringify(snapshot), original);
        const movable = JSON.parse(original);
        movable.items[0].equipped = false;
        panel.inventory = movable;
        waitForRendering(panel);
        const looseItem = findChild(panel, "inventoryName_armor");
        const pouch = findChild(panel, "inventoryDrop_pouch");
        mousePress(looseItem, 5, 5);
        mouseMove(looseItem, 30, 5);
        mouseMove(pouch, 45, 16);
        mouseRelease(pouch, 45, 16);
        tryCompare(moveSpy, "count", 2);
        compare(Array.from(moveSpy.signalArguments[1]), ["armor", "pouch.container"]);
    }
    function test_addRootContainer() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        containerSpy.target = panel; containerSpy.clear();
        waitForRendering(panel);
        openContainerPicker(panel);
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
        openContainerPicker(panel);
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
        const pool = findChild(panel, "inventoryUsagePool_armor_charges");
        compare(findChild(pool, "inventoryUsagePoolBalance").text, "Cariche (charges): 2 / 5");
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

    function test_loadReadsCategoryAndEffectsFromSnapshot() {
        const panel = createTemporaryObject(component, testCase, {inventory: fixture(), width: 1000,
            encumbrance: {totalWeightGrams: 13710, category: InventoryPanel.Medium,
                lightLoadMaxGrams: 10000, mediumLoadMaxGrams: 20000, heavyLoadMaxGrams: 30000,
                effects: {maximumDexterityBonus: 3, armorCheckPenalty: 3, reducesMovement: true}}});
        waitForRendering(panel);
        compare(findChild(panel, "loadCategory").text, "Carico medio");
        const equipment = findChild(panel, "equipmentDiagram");
        const loadCard = findChild(panel, "inventoryLoadCard");
        compare(loadCard.mapToItem(panel, 0, 0).x, equipment.mapToItem(panel, 0, 0).x);
        compare(loadCard.width, equipment.width);
        verify(loadCard.mapToItem(panel, 0, 0).y >= equipment.mapToItem(panel, 0, equipment.height).y);
        const details = findChild(panel, "loadDetails");
        verify(!details.visible);
        findChild(panel, "loadInfo").click();
        verify(details.visible);
        verify(details.summaryText.includes("Penalità alle prove: −3"));
        verify(details.summaryText.includes("Velocità ridotta"));
        findChild(panel, "loadInfo").click();
        verify(!details.visible);
    }
    function test_unlimitedVolumeInItemInfo() {
        const snapshot = fixture();
        snapshot.containers[1].currentContentsVolumeMilliliters = 1500;
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        verify(!findChild(panel, "inventoryVolumeCapacity_bag").visible);
        const info = findChild(panel, "inventoryInfo_bag");
        mouseMove(info, 10, 10);
        mouseClick(info);
        const details = findChild(panel, "inventoryDetails_bag");
        verify(details.visible);
        verify(details.summaryText.includes("Volume del contenuto: " + panel.volume(1500) + " (senza limite)"));
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
        snapshot.items[1].intrinsicWeightGrams = 1000;
        snapshot.items[2].intrinsicWeightGrams = 200;
        snapshot.items[3].intrinsicWeightGrams = 500;
        snapshot.items[3].intrinsicVolumeMilliliters = 500;
        snapshot.containers[1].currentContentsWeightGrams = 1000;
        snapshot.containers[1].maximumContentsWeightGrams = 5000;
        snapshot.containers[1].currentContentsVolumeMilliliters = 250;
        snapshot.containers[1].maximumContentsVolumeMilliliters = 1000;
        snapshot.containers[0].currentContentsVolumeMilliliters = 500;
        snapshot.containers[3].currentContentsVolumeMilliliters = 0;
        snapshot.containers[3].maximumContentsVolumeMilliliters = 0;
        snapshot.containers[2].currentContentsWeightGrams = 500;
        snapshot.containers[2].maximumContentsWeightGrams = 2000;
        snapshot.containers[0].currentContentsWeightGrams = 7000;
        snapshot.containers[3].currentContentsWeightGrams = 0;
        snapshot.containers[3].maximumContentsWeightGrams = 0;
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        compare(findChild(panel, "inventoryWeight_bag").text, panel.weight(1000));
        compare(findChild(panel, "inventoryWeight_pouch").text, panel.weight(200));
        verify(!findChild(panel, "inventoryWeight_worn").visible);
        compare(findChild(panel, "inventoryWeight_potion").text, panel.weight(250));
        const bagCapacity = findChild(panel, "inventoryCapacity_bag");
        compare(bagCapacity.fillAmount, 0.2);
        const volume = findChild(panel, "inventoryVolumeCapacity_bag");
        verify(volume.visible);
        compare(volume.fillAmount, 0.25);
        verify(volume.tooltipText.includes("25%"));
        verify(!findChild(panel, "inventoryVolumeCapacity_worn").visible);
        verify(findChild(panel, "inventoryVolumeCapacity_home").visible);
        compare(findChild(panel, "inventoryContentsWeight_bag").text, panel.weight(1000).replace(" kg", "\nkg"));
        compare(findChild(panel, "inventoryContentsWeight_worn").text, panel.weight(7000).replace(" kg", "\nkg"));
        compare(bagCapacity.tooltipText, panel.weight(1000) + " / " + panel.weight(5000) + " — 20%");
        compare(findChild(panel, "inventoryCapacity_pouch").fillAmount, 0.25);
        verify(!findChild(panel, "inventoryCapacity_potion").visible);
        const unlimited = findChild(panel, "inventoryCapacity_worn");
        verify(unlimited.unlimited);
        compare(unlimited.tooltipText, panel.weight(7000) + " / ∞");
        const empty = findChild(panel, "inventoryCapacity_home");
        verify(!empty.unlimited);
        compare(empty.fillAmount, 0);
        compare(empty.tooltipText, panel.weight(0) + " / " + panel.weight(0) + " — 0%");
        compare(panel.capacityFraction({currentContentsWeightGrams: 5000, maximumContentsWeightGrams: 5000}), 1);
        verify(panel.itemInfoRows("potion").some(row => row.text === "Peso unitario: " + panel.weight(250)));
        verify(panel.itemInfoRows("potion").some(row => row.text === "Volume unitario: " + panel.volume(250)));
        verify(!panel.itemInfoRows("potion").some(row => row.text.startsWith("Peso complessivo:")));
    }
    function test_weightExclusionIndicators() {
        const snapshot = fixture();
        snapshot.containers[0].contributesToCarriedWeight = true;
        snapshot.containers[1].contributesToCarriedWeight = false;
        snapshot.containers[2].ignoresContentsWeight = true;
        snapshot.containers[3].contributesToCarriedWeight = false;
        const panel = createTemporaryObject(component, testCase, {inventory: snapshot, width: 1000});
        waitForRendering(panel);
        verify(!findChild(panel, "inventoryCapacity_worn").dashed);
        verify(!findChild(panel, "inventoryCapacity_bag").dashed);
        verify(!findChild(panel, "inventoryCapacity_potion").visible);
        const capacity = findChild(panel, "inventoryCapacity_pouch");
        verify(capacity.dashed);
        tryCompare(findChild(capacity, "dashedCapacityRing"), "status", Image.Ready);
        verify(capacity.tooltipText.includes("conta solo il contenitore"));
        const home = findChild(panel, "inventoryCapacity_home");
        verify(home.dashed);
        verify(home.tooltipText.includes("Non contribuisce al peso trasportato."));
        verify(findChild(panel, "inventoryWeightExclusion_pouch") === null);
        panel.width = 780;
        const updated = JSON.parse(JSON.stringify(snapshot));
        updated.items[2].name = "Borsello speciale dal nome molto lungo";
        panel.inventory = updated;
        waitForRendering(panel);
        const pouchRow = findChild(panel, "inventoryRow_pouch");
        const indicator = findChild(panel, "inventoryCapacity_pouch");
        verify(indicator.mapToItem(pouchRow, indicator.width, 0).x <= pouchRow.width);
    }
}
