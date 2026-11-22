pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    enum Slot { Ring, Armor, Belt, Neck, Body, Headband, Hands, Eyes, Feet, Wrists, Shield, Shoulders, Head, Chest }
    enum LoadCategory { Light, Medium, Heavy, Overloaded }
    required property var inventory
    property var encumbrance: null
    property Component equipmentFooter
    property bool encumbranceBelowItems: false
    signal addContainerRequested(var definition)
    signal addCatalogItemRequested()
    signal addCustomItemRequested(var definition)
    readonly property alias customItemPopup: customItemEditor
    signal moveItemRequested(string itemId, string containerId)
    signal itemQuantityChangeRequested(string itemId, int quantity)
    signal mergeCoinStacksRequested(string sourceItemId, string targetItemId)
    signal splitCoinStackRequested(string itemId, int quantity)
    signal restoreItemUsagePoolRequested(string itemId, string poolId, int amount)
    signal refillItemUsagePoolRequested(string itemId, string poolId)
    property var expandedItemDetails: []

    function setItemDetailsExpanded(itemId, expanded) {
        if (expanded === expandedItemDetails.includes(itemId)) return;
        expandedItemDetails = expanded ? expandedItemDetails.concat(itemId) : expandedItemDetails.filter(id => id !== itemId);
    }
    signal reorderItemRequested(string itemId, var beforeItemId)
    signal moveAndReorderItemRequested(string itemId, string containerId, var beforeItemId)
    function reorderDestination(sourceId, targetId, after) {
        const source = item(sourceId);
        if (sourceId === targetId || !source || source.equipped) return undefined;
        const container = (inventory?.containers ?? []).find(entry => entry.itemIds.includes(targetId));
        if (!container) return undefined;
        const moving = !container.itemIds.includes(sourceId);
        if (moving && !canRequestMove(sourceId, container.id)) return undefined;
        const remaining = container.itemIds.filter(id => id !== sourceId);
        const beforeItemId = after ? (remaining[remaining.indexOf(targetId) + 1] ?? null) : targetId;
        return {containerId: container.id, beforeItemId: beforeItemId, moving: moving};
    }
    function isCoinStack(entry) {
        return !!entry && (entry.coinDenomination != null
            || ["copperCoin", "silverCoin", "goldCoin", "platinumCoin"].includes(entry.itemDefinitionId));
    }
    function canMergeCoinStacks(sourceId, targetId) {
        const source = item(sourceId);
        const target = item(targetId);
        return sourceId !== targetId && isCoinStack(source) && isCoinStack(target)
            && !source.equipped && !target.equipped && source.itemDefinitionId === target.itemDefinitionId
            && !!source.containerId && source.containerId === target.containerId
            && source.quantity > 0 && target.quantity > 0 && source.quantity + target.quantity <= 2147483647;
    }
    signal unequipItemRequested(string itemId, string containerId)
    signal equipItemRequested(string itemId, string replacedItemId, string sourceContainerId)
    property string pendingEquipItemId: ""
    property int pendingEquipSlot: -1
    property string draggedItemId: ""
    readonly property var draggedItem: draggedItemId ? item(draggedItemId) : null
    function canEquipInSlot(entry, slotView) {
        return !!entry && !!slotView && !entry.equipped && entry.quantity === 1
            && entry.equipmentSlot === slotView.slot && slotView.capacity > 0;
    }
    function requestEquip(itemId, slotType) {
        const entry = item(itemId);
        const target = slot(slotType);
        if (!canEquipInSlot(entry, target)) return false;
        const occupants = target.itemIds ?? [];
        if (occupants.length < target.capacity)
            equipItemRequested(itemId, "", entry.containerId ?? "");
        else if (occupants.length === 1)
            equipItemRequested(itemId, occupants[0], entry.containerId ?? "");
        else {
            pendingEquipItemId = itemId;
            pendingEquipSlot = slotType;
            replacementPicker.open();
        }
        return true;
    }
    Dialog {
        id: customItemEditor
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Nuovo oggetto personalizzato")
        modal: true
        width: Math.min(560, (parent?.width ?? root.width) - 24)
        height: Math.min(670, (parent?.height ?? 720) - 24)
        property string error: ""
        onAboutToShow: {
            customName.clear(); customDescription.clear(); customTags.clear(); customAdvanced.clear();
            customWeight.text = "0"; customVolume.text = "0"; customSlot.currentIndex = 0; error = "";
        }
        function submit() {
            const weight = Number(customWeight.text);
            const volume = Number(customVolume.text);
            if (!customName.text.trim() || !customWeight.text.trim() || !customVolume.text.trim()
                    || !Number.isSafeInteger(weight) || weight < 0 || !Number.isSafeInteger(volume) || volume < 0) {
                error = qsTr("Inserisci un nome e valori interi non negativi per peso e volume.");
                return;
            }
            let advanced;
            try {
                advanced = JSON.parse(customAdvanced.text.trim() || "{}");
                if (!advanced || typeof advanced !== "object" || Array.isArray(advanced)
                        || Object.keys(advanced).some(key => !["container", "effects", "choices", "usagePools", "coinDenomination"].includes(key)))
                    throw new Error("Invalid advanced item fields");
            } catch (exception) {
                error = qsTr("Le proprietà avanzate devono essere un oggetto JSON con i soli campi indicati.");
                return;
            }
            const definition = Object.assign(advanced, {
                name: customName.text.trim(), description: customDescription.text.trim(),
                weightGrams: weight, volumeMilliliters: volume,
                tags: customTags.text.split(",").map(tag => tag.trim()).filter(tag => tag.length > 0),
                slot: customSlot.currentIndex === 0 ? null : root.slotOrder[customSlot.currentIndex - 1]
            });
            close();
            root.addCustomItemRequested(definition);
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: customItemEditor.reject() }
            Button { objectName: "submitCustomItem"; text: qsTr("Crea oggetto"); onClicked: customItemEditor.submit() }
        }
        contentItem: ScrollView {
            id: customFormScroll
            contentWidth: availableWidth
            ColumnLayout {
                width: customFormScroll.availableWidth
                spacing: 8
                Label { text: qsTr("Nome") }
                TextField { id: customName; objectName: "customItemName"; Layout.fillWidth: true }
                Label { text: qsTr("Descrizione") }
                TextArea { id: customDescription; Layout.fillWidth: true; Layout.preferredHeight: 70; wrapMode: Text.Wrap }
                Label { text: qsTr("Peso unitario (g)") }
                TextField { id: customWeight; objectName: "customItemWeight"; Layout.fillWidth: true; inputMethodHints: Qt.ImhDigitsOnly }
                Label { text: qsTr("Volume unitario (ml)") }
                TextField { id: customVolume; Layout.fillWidth: true; inputMethodHints: Qt.ImhDigitsOnly }
                Label { text: qsTr("Etichette (separate da virgole)") }
                TextField { id: customTags; Layout.fillWidth: true }
                Label { text: qsTr("Slot di equipaggiamento") }
                ComboBox { id: customSlot; Layout.fillWidth: true; model: [qsTr("Nessuno")].concat(root.slotOrder.map(type => root.slotName(type))) }
                Label { text: qsTr("Proprietà avanzate (JSON, facoltative)"); font.bold: true }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("Contenitore (container), effetti (effects), scelte (choices), riserve di utilizzi (usagePools), moneta (coinDenomination).")
                }
                TextArea { id: customAdvanced; Layout.fillWidth: true; Layout.preferredHeight: 100; placeholderText: "{}"; wrapMode: Text.Wrap }
                Label { Layout.fillWidth: true; visible: !!text; text: customItemEditor.error; wrapMode: Text.Wrap; color: "#9f3030" }
            }
        }
    }
    Dialog {
        id: replacementPicker
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Quale oggetto vuoi sostituire?")
        modal: true
        standardButtons: Dialog.Cancel
        contentItem: Column {
            spacing: 4
            Repeater {
                model: root.slot(root.pendingEquipSlot)?.itemIds ?? []
                delegate: Button {
                    required property string modelData
                    width: parent.width
                    text: root.item(modelData)?.name ?? modelData
                    onClicked: {
                        const entry = root.item(root.pendingEquipItemId);
                        if (root.canEquipInSlot(entry, root.slot(root.pendingEquipSlot)))
                            root.equipItemRequested(entry.id, modelData, entry.containerId ?? "");
                        replacementPicker.close();
                    }
                }
            }
        }
    }
    function canRequestMove(itemId, containerId, fromEquipment = false) {
        const entry = item(itemId);
        const containers = inventory?.containers ?? [];
        const destination = containers.find(container => container.id === containerId);
        if (!entry || !!entry.equipped !== fromEquipment || !destination
                || (!fromEquipment && destination.itemIds.includes(itemId))) return false;
        const visited = new Set();
        let current = destination;
        while (current?.ownerItemId) {
            if (current.ownerItemId === itemId || visited.has(current.id)) return false;
            visited.add(current.id);
            const ownerId = current.ownerItemId;
            current = containers.find(container => container.itemIds.includes(ownerId));
        }
        return true;
    }
    readonly property alias containerPopup: containerPicker
    onVisibleChanged: if (!visible) { draggedItemId = ""; containerPicker.close(); replacementPicker.close(); }
    onInventoryChanged: { draggedItemId = ""; if (containerPicker) containerPicker.close(); if (replacementPicker) replacementPicker.close(); }
    Dialog {
        id: containerPicker
        parent: Overlay.overlay
        title: qsTr("Nuovo contenitore")
        modal: true
        width: Math.min(440, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        onAboutToShow: { containerName.clear(); carriedWeight.checked = false; possessionEffects.checked = true; }
        onOpened: containerName.forceActiveFocus()
        function submit() {
            const name = containerName.text.trim();
            if (!name) return;
            root.addContainerRequested({name: name, contributesToCarriedWeight: carriedWeight.checked, allowsPossessionEffects: possessionEffects.checked});
            close();
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: containerPicker.reject() }
            Button { objectName: "confirmInventoryContainer"; text: qsTr("Aggiungi"); enabled: !!containerName.text.trim(); onClicked: containerPicker.submit() }
        }
        contentItem: Column {
            spacing: 10
            Label { text: qsTr("Nome"); font.bold: true }
            TextField { id: containerName; objectName: "inventoryContainerName"; width: parent.width; placeholderText: qsTr("Es. Casa, Deposito…"); onAccepted: containerPicker.submit() }
            CheckBox { id: carriedWeight; objectName: "inventoryContainerCarriedWeight"; text: qsTr("Conta nel peso trasportato"); width: parent.width }
            CheckBox { id: possessionEffects; objectName: "inventoryContainerPossessionEffects"; text: qsTr("Attiva gli effetti degli oggetti posseduti"); width: parent.width }
        }
    }
    property var collapsedContainers: []
    readonly property var slotOrder: [InventoryPanel.Head, InventoryPanel.Eyes, InventoryPanel.Neck, InventoryPanel.Chest, InventoryPanel.Body, InventoryPanel.Hands, InventoryPanel.Ring,
        InventoryPanel.Headband, InventoryPanel.Shoulders, InventoryPanel.Armor, InventoryPanel.Belt, InventoryPanel.Wrists, InventoryPanel.Shield, InventoryPanel.Feet]
    function slotAnchor(type) {
        const points = [[.14,.54],[.57,.30],[.55,.42],[.48,.16],[.45,.36],[.5,.035],[.13,.51],[.5,.075],[.6,.975],[.84,.46],[.86,.51],[.7,.19],[.5,.01],[.45,.25]];
        return Qt.point(silhouette.x + (silhouette.width - silhouette.paintedWidth) / 2 + points[type][0] * silhouette.paintedWidth,
            silhouette.y + (silhouette.height - silhouette.paintedHeight) / 2 + points[type][1] * silhouette.paintedHeight);
    }
    implicitWidth: 1000
    implicitHeight: content.height
    function slotName(slot) {
        return [qsTr("Anelli"), qsTr("Armatura"), qsTr("Cintura"), qsTr("Collo"), qsTr("Corpo"), qsTr("Fronte"), qsTr("Mani"), qsTr("Occhi"), qsTr("Piedi"), qsTr("Polsi"), qsTr("Scudo"), qsTr("Spalle"), qsTr("Testa"), qsTr("Torace")][slot] ?? "—";
    }
    function item(id) { return (inventory?.items ?? []).find(entry => entry.id === id); }
    function slot(type) { return (inventory?.equipmentSlots ?? []).find(entry => entry.slot === type); }
    function weight(grams) { return grams === undefined ? "—" : (grams / 1000).toLocaleString(Qt.locale("it_IT"), "f", grams % 1000 ? 2 : 0) + " kg"; }
    function loadName(category) {
        switch (category) {
        case InventoryPanel.Light: return qsTr("Carico leggero");
        case InventoryPanel.Medium: return qsTr("Carico medio");
        case InventoryPanel.Heavy: return qsTr("Carico pesante");
        case InventoryPanel.Overloaded: return qsTr("Sovraccarico");
        default: return "";
        }
    }
    function loadEffectRows(effects) {
        const rows = [{kind: InfoDetails.Heading, text: qsTr("Effetti del carico")}];
        function add(text) { rows.push({kind: InfoDetails.Entry, text: text}); }
        if (effects?.losesDexterityBonusToArmorClass) add(qsTr("Bonus di Destrezza alla CA perso"));
        else if (effects?.maximumDexterityBonus != null)
            add(qsTr("Bonus massimo di Destrezza alla CA: +%1").arg(effects.maximumDexterityBonus));
        if (effects?.armorCheckPenalty > 0)
            add(qsTr("Penalità alle prove: −%1").arg(effects.armorCheckPenalty));
        if (effects?.movementSpeedLimitUnits != null)
            add(qsTr("Limite di velocità: %1 m").arg((effects.movementSpeedLimitUnits * 1.5).toLocaleString(Qt.locale("it_IT"))));
        else if (effects?.reducesMovement) add(qsTr("Velocità ridotta"));
        if (effects?.preventsRunning) add(qsTr("Non puoi correre"));
        else if (effects?.runMultiplierPenalty > 0)
            add(qsTr("Moltiplicatore di corsa: −%1").arg(effects.runMultiplierPenalty));
        if (rows.length === 1) add(qsTr("Nessuna penalità di carico."));
        return rows;
    }
    function volume(milliliters) { return milliliters == null ? "—" : (milliliters / 1000).toLocaleString(Qt.locale("it_IT"), "f", milliliters % 1000 ? 3 : 0) + " L"; }
    function rowWeight(node) {
        const entry = item(node.id);
        return weight(entry?.intrinsicWeightGrams !== undefined && entry.quantity > 0 ? entry.intrinsicWeightGrams / entry.quantity : undefined);
    }
    function capacityFraction(container) {
        return container?.maximumContentsWeightGrams > 0 ? Math.min(1, Math.max(0, container.currentContentsWeightGrams / container.maximumContentsWeightGrams)) : 0;
    }
    function capacityText(container) {
        const maximum = container?.maximumContentsWeightGrams;
        return maximum === undefined || maximum === null ? weight(container?.currentContentsWeightGrams) + " / ∞"
            : weight(container.currentContentsWeightGrams) + " / " + weight(maximum) + " — " + Math.round(capacityFraction(container) * 100) + "%";
    }
    function weightExclusionText(node) {
        const notes = [];
        if (node.rootContainer && node.container?.contributesToCarriedWeight === false) notes.push(qsTr("Non contribuisce al peso trasportato."));
        if (node.container?.ignoresContentsWeight === true) notes.push(qsTr("Il contenuto non aggiunge peso; conta solo il contenitore."));
        return notes.join("\n");
    }
    function toggleContainer(id) { collapsedContainers = collapsedContainers.includes(id) ? collapsedContainers.filter(value => value !== id) : [...collapsedContainers, id]; }
    function itemInfoRows(id) {
        const entry = item(id);
        if (!entry) return [];
        const lines = [{kind: InfoDetails.Heading, text: entry.name}];
        if (entry.description) lines.push({kind: InfoDetails.Entry, text: entry.description});
        lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: qsTr("Dettagli")},
            {kind: InfoDetails.Entry, text: qsTr("Quantità: %1").arg(entry.quantity)});
        if (entry.intrinsicWeightGrams !== undefined && entry.quantity > 0) lines.push({kind: InfoDetails.Entry, text: qsTr("Peso unitario: %1").arg(weight(entry.intrinsicWeightGrams / entry.quantity))});
        if (entry.intrinsicVolumeMilliliters != null && entry.quantity > 0)
            lines.push({kind: InfoDetails.Entry, text: qsTr("Volume unitario: %1").arg(volume(entry.intrinsicVolumeMilliliters / entry.quantity))});
        for (const choice of entry.choices ?? []) {
            lines.push({kind: InfoDetails.Group, text: choice.prompt});
            for (const option of choice.options ?? []) if (option.selected) lines.push({kind: InfoDetails.Entry, text: option.name});
        }
        return lines;
    }
    function rows() {
        const result = [];
        const containers = inventory?.containers ?? [];
        function append(container, depth, ancestors) {
            if (!container || ancestors.includes(container.id) || root.collapsedContainers.includes(container.id)) return;
            for (const id of container.itemIds ?? []) {
                const entry = root.item(id);
                if (!entry) continue;
                const owned = containers.find(candidate => candidate.id === entry.ownedContainerId);
                result.push({id: entry.id, depth: depth, name: entry.name, quantity: entry.quantity,
                    weight: entry.effectiveWeightGrams, equipped: entry.equipped, container: owned ?? null});
                if (owned) append(owned, depth + 1, [...ancestors, container.id]);
            }
        }
        for (const container of containers.filter(entry => !entry.ownerItemId)) {
            result.push({id: container.id, depth: 0, name: container.name, rootContainer: true, container: container});
            append(container, 0, []);
        }
        return result;
    }
    function tree() {
        const roots = [];
        let parents = [];
        for (const entry of rows()) {
            const node = Object.assign({}, entry, {children: []});
            if (entry.rootContainer) {
                roots.push(node);
                parents = [node];
            } else {
                parents[entry.depth].children.push(node);
                parents[entry.depth + 1] = node;
                parents.length = entry.depth + 2;
            }
        }
        return roots;
    }
    Column {
        id: content
        width: root.width
        spacing: 16
        Label { text: qsTr("Inventario ed equipaggiamento"); font.pixelSize: 20; font.bold: true; color: "#253947"; width: parent.width; wrapMode: Text.Wrap }
        GridLayout {
            width: parent.width
            columns: width >= 720 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16
            Column {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 440
                spacing: 12
                Rectangle {
                    id: equipment
                    objectName: "equipmentDiagram"
                    width: parent.width
                    implicitHeight: 610
                    color: "#e9eef1"; border.color: "#c6cfd5"; radius: 8
                    Label { x: 16; y: 16; text: qsTr("Equipaggiamento"); font.pixelSize: 18; font.bold: true; color: "#253947" }
                    Image {
                        id: silhouette
                        objectName: "equipmentSilhouette"
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 78
                        width: Math.max(100, Math.min(170, parent.width - 204))
                        height: 480
                        source: "character-silhouette.svg"
                        sourceSize.width: 360
                        sourceSize.height: 832
                        fillMode: Image.PreserveAspectFit
                        onPaintedWidthChanged: slotConnections.requestPaint()
                        onPaintedHeightChanged: slotConnections.requestPaint()
                    }
                    Canvas {
                        id: slotConnections
                        objectName: "equipmentConnections"
                        anchors.fill: parent
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = "#829bab";
                            ctx.fillStyle = "#52768e";
                            ctx.lineWidth = 1;
                            const cardWidth = Math.max(90, (equipment.width - 200) / 2);
                            root.slotOrder.forEach((type, index) => {
                                const left = index < 7;
                                const startX = left ? 12 + cardWidth : equipment.width - cardWidth - 12;
                                const startY = 91 + (index % 7) * 76;
                                const point = root.slotAnchor(type);
                                ctx.beginPath();
                                ctx.moveTo(startX, startY);
                                ctx.lineTo(startX + (left ? 6 : -6), startY);
                                ctx.lineTo(point.x, point.y);
                                ctx.stroke();
                                ctx.beginPath();
                                ctx.arc(point.x, point.y, 2.2, 0, 2 * Math.PI);
                                ctx.fill();
                            });
                        }
                    }
                    Repeater {
                        model: root.slotOrder
                        Rectangle {
                            id: slotCard
                            required property int index
                            required property int modelData
                            readonly property var slotView: root.slot(modelData)
                            objectName: "equipmentSlot_" + modelData
                            x: index < 7 ? 12 : equipment.width - width - 12
                            y: 58 + (index % 7) * 76
                            width: Math.max(90, (equipment.width - 200) / 2)
                            height: 66
                            readonly property bool compatible: root.canEquipInSlot(root.draggedItem, slotView)
                            opacity: !root.draggedItem || compatible ? 1 : 0.38
                            color: compatible ? "#dcebf4" : "#fafbfc"
                            border.color: compatible ? "#52768e" : "#c6cfd5"
                            border.width: compatible ? 2 : 1
                            radius: 5
                            DropArea {
                                id: equipTarget
                                anchors.fill: parent
                                keys: ["inventory-item"]
                                function sourceId(source) { return source?.itemId; }
                                onEntered: drag => {
                                    drag.accepted = root.canEquipInSlot(root.item(sourceId(drag.source)), slotCard.slotView);
                                }
                                onDropped: drop => {
                                    if (root.requestEquip(sourceId(drop.source), slotCard.modelData)) drop.acceptProposedAction();
                                }
                            }
                            Column {
                                x: 8; y: 7; width: parent.width - 16; spacing: 3
                                Label { width: parent.width; text: root.slotName(slotCard.modelData); font.pixelSize: 12; font.bold: true; color: "#536976" }
                                Repeater {
                                    model: slotCard.slotView?.itemIds ?? []
                                    Label {
                                        id: equippedLabel
                                        required property string modelData
                                        objectName: "equippedItem_" + modelData
                                        width: parent.width
                                        text: root.item(modelData)?.name ?? modelData
                                        textFormat: Text.PlainText
                                        elide: Text.ElideRight
                                        font.pixelSize: 13
                                        color: "#253947"
                                        DragHandler {
                                            id: equippedDrag
                                            target: null
                                            onActiveChanged: {
                                                if (active) equippedMarker.Drag.active = true;
                                                else if (equippedMarker.Drag.active) equippedMarker.Drag.drop();
                                            }
                                            onCanceled: equippedMarker.Drag.cancel()
                                        }
                                        Item {
                                            id: equippedMarker
                                            readonly property string itemId: equippedLabel.modelData
                                            readonly property bool fromEquipment: true
                                            x: equippedDrag.centroid.position.x
                                            y: equippedDrag.centroid.position.y
                                            Drag.keys: ["inventory-unequip"]
                                            Drag.supportedActions: Qt.MoveAction
                                            Drag.proposedAction: Qt.MoveAction
                                        }
                                        Rectangle {
                                            parent: Overlay.overlay
                                            visible: equippedDrag.active
                                            z: 1
                                            readonly property point pointer: visible && parent
                                                ? equippedLabel.mapToItem(parent, equippedDrag.centroid.position.x, equippedDrag.centroid.position.y)
                                                : Qt.point(0, 0)
                                            x: pointer.x + 12; y: pointer.y + 10
                                            width: Math.min(240, equippedPreview.implicitWidth + 20)
                                            height: equippedPreview.implicitHeight + 12
                                            color: "#fafbfc"; border.color: "#52768e"; radius: 5
                                            Label {
                                                id: equippedPreview
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                text: equippedLabel.text
                                                textFormat: Text.PlainText
                                                wrapMode: Text.Wrap
                                                color: "#253947"
                                            }
                                        }
                                    }
                                }
                                Label {
                                    width: parent.width
                                    visible: !(slotCard.slotView?.itemIds ?? []).length
                                    text: slotCard.slotView ? (slotCard.slotView.capacity ? qsTr("Vuoto") : qsTr("Non disponibile")) : "—"
                                    textFormat: Text.PlainText; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight; font.pixelSize: 13; color: "#253947"
                                }
                            }
                            ToolTip.visible: slotHover.hovered
                            ToolTip.text: (slotView?.itemIds ?? []).map(id => root.item(id)?.name ?? id).join("\n")
                            HoverHandler { id: slotHover }
                        }
                    }
                }
                Loader {
                    width: parent.width
                    active: !root.encumbranceBelowItems && !!root.encumbrance
                    visible: active
                    sourceComponent: Component {
                        id: loadCard
                        InfoFrame {
                            objectName: "inventoryLoadCard"
                            width: parent.width
                            visible: !!root.encumbrance
                            height: loadContent.implicitHeight + 20
                            Column {
                                id: loadContent
                                x: 10; y: 10
                                width: parent.width - 20
                                spacing: 6
                                RowLayout {
                                    width: parent.width
                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Peso trasportato: %1 / %2").arg(root.weight(root.encumbrance?.totalWeightGrams))
                                            .arg(root.weight(root.encumbrance?.heavyLoadMaxGrams))
                                        wrapMode: Text.Wrap
                                        font.bold: true
                                        color: "#253947"
                                    }
                                    Label {
                                        objectName: "loadCategory"
                                        text: root.loadName(root.encumbrance?.category)
                                        wrapMode: Text.Wrap
                                        color: "#365f75"
                                    }
                                    InfoButton { id: loadInfo; objectName: "loadInfo"; Accessible.name: qsTr("Effetti del carico") }
                                }
                                ProgressBar {
                                    width: parent.width
                                    from: 0
                                    to: Math.max(1, root.encumbrance?.heavyLoadMaxGrams ?? 0)
                                    value: Math.min(to, root.encumbrance?.totalWeightGrams ?? 0)
                                    Accessible.name: qsTr("Peso rispetto alla soglia di carico pesante")
                                }
                                RowLayout {
                                    width: parent.width
                                    Repeater {
                                        model: [
                                            {name: qsTr("Leggero"), grams: root.encumbrance?.lightLoadMaxGrams},
                                            {name: qsTr("Medio"), grams: root.encumbrance?.mediumLoadMaxGrams},
                                            {name: qsTr("Pesante"), grams: root.encumbrance?.heavyLoadMaxGrams}
                                        ]
                                        Label {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignHCenter
                                            text: modelData.name + "\n" + root.weight(modelData.grams)
                                            color: "#536976"
                                        }
                                    }
                                }
                                InfoDetails {
                                    objectName: "loadDetails"
                                    width: parent.width
                                    visible: loadInfo.checked
                                    framed: false
                                    rows: root.loadEffectRows(root.encumbrance?.effects)
                                }
                            }
                        }
                    }
                }
                Loader {
                    width: parent.width
                    active: root.equipmentFooter !== null
                    visible: active
                    sourceComponent: root.equipmentFooter
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 450
                implicitHeight: belongings.height + 32
                color: "#fafbfc"; border.color: "#d2dbe1"; radius: 8
                Column {
                    id: belongings
                    x: 16; y: 16; width: parent.width - 32; spacing: 12
                    RowLayout {
                        width: parent.width
                        Label { Layout.fillWidth: true; text: qsTr("Oggetti e contenitori"); font.pixelSize: 18; font.bold: true; color: "#253947"; wrapMode: Text.Wrap }
                        ToolButton {
                            objectName: "addInventoryEntry"
                            readonly property alias picker: addEntryMenu
                            text: "+"
                            font.pixelSize: 23
                            Accessible.name: qsTr("Aggiungi all’inventario")
                            onClicked: addEntryMenu.open()
                            Menu {
                                id: addEntryMenu
                                y: parent.height
                                Menu {
                                    objectName: "addInventoryItem"
                                    title: qsTr("Oggetto")
                                    MenuItem { text: qsTr("Catalogo"); onTriggered: root.addCatalogItemRequested() }
                                    MenuItem { text: qsTr("Personalizzato"); onTriggered: customItemEditor.open() }
                                }
                                MenuItem { objectName: "addInventoryContainer"; text: qsTr("Contenitore"); onTriggered: containerPicker.open() }
                            }
                        }
                    }
                    Label {
                        objectName: "carriedWeight"
                        visible: !root.encumbrance
                        text: qsTr("Peso trasportato: %1").arg(root.weight(root.inventory?.totalCarriedWeightGrams))
                        color: "#536976"
                    }
                    Label { visible: !root.rows().length; text: qsTr("Inventario vuoto."); color: "#687c88" }
                    Column {
                        width: parent.width
                        spacing: 6
                        Repeater {
                            model: root.tree()
                            InventoryCard {
                                required property var modelData
                                width: parent.width
                                node: modelData
                                panel: root
                            }
                        }
                    }
                    Loader {
                        width: parent.width
                        active: root.encumbranceBelowItems && !!root.encumbrance
                        visible: active
                        sourceComponent: loadCard
                    }
                }
            }
        }
    }
}
