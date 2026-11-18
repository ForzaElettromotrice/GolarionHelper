pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    enum Slot { Ring, Armor, Belt, Neck, Body, Headband, Hands, Eyes, Feet, Wrists, Shield, Shoulders, Head, Chest }
    required property var inventory
    signal addContainerRequested(var definition)
    readonly property alias containerPopup: containerPicker
    onVisibleChanged: if (!visible) containerPicker.close()
    onInventoryChanged: if (containerPicker) containerPicker.close()
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
    function rowWeight(node) {
        return node.container ? weight(node.container.currentContentsWeightGrams) + " / "
            + (node.container.maximumContentsWeightGrams === undefined || node.container.maximumContentsWeightGrams === null ? "∞" : weight(node.container.maximumContentsWeightGrams)) : weight(node.weight);
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
        for (const pool of entry.usagePools ?? [])
            lines.push({kind: InfoDetails.Entry, text: qsTr("Utilizzi (%1): %2 / %3").arg(pool.id).arg(pool.remaining).arg(pool.capacity)});
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
            Rectangle {
                id: equipment
                objectName: "equipmentDiagram"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 440
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
                        color: "#fafbfc"; border.color: "#c6cfd5"; radius: 5
                        Column {
                            x: 8; y: 7; width: parent.width - 16; spacing: 3
                            Label { width: parent.width; text: root.slotName(slotCard.modelData); font.pixelSize: 12; font.bold: true; color: "#536976" }
                            Label {
                                width: parent.width
                                text: slotCard.slotView ? ((slotCard.slotView.itemIds ?? []).map(id => root.item(id)?.name ?? id).join(" · ") || (slotCard.slotView.capacity ? qsTr("Vuoto") : qsTr("Non disponibile"))) : "—"
                                textFormat: Text.PlainText; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight; font.pixelSize: 13; color: "#253947"
                            }
                        }
                        ToolTip.visible: slotHover.hovered
                        ToolTip.text: (slotView?.itemIds ?? []).map(id => root.item(id)?.name ?? id).join("\n")
                        HoverHandler { id: slotHover }
                    }
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
                        ToolButton { objectName: "addInventoryContainer"; text: "+"; font.pixelSize: 23; Accessible.name: qsTr("Aggiungi contenitore"); onClicked: containerPicker.open() }
                    }
                    Label { objectName: "carriedWeight"; text: qsTr("Peso trasportato: %1").arg(root.weight(root.inventory?.totalCarriedWeightGrams)); color: "#536976" }
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
                }
            }
        }
    }
}
