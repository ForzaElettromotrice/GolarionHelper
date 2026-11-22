pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property var node
    required property var panel
    objectName: "inventoryRow_" + node.id
    implicitHeight: body.height + 16
    color: destination.containsDrag ? "#dcebf4" : node.rootContainer ? "#e9eef1" : "#ffffff"
    border.color: destination.containsDrag ? "#52768e" : "#d2dbe1"
    radius: 5
    function draggedItemId(source) { return source?.itemId; }
    function fromEquipment(source) { return source?.fromEquipment === true; }
    readonly property bool canSplitCoins: root.node.quantity > 1 && !root.node.equipped
        && root.panel.isCoinStack(root.panel.item(root.node.id))
    Dialog {
        id: splitPicker
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Dividi pila")
        onAboutToShow: splitQuantity.value = 1
        function submit() {
            if (!root.canSplitCoins || splitQuantity.value >= root.node.quantity) return;
            const amount = splitQuantity.value;
            close();
            root.panel.splitCoinStackRequested(root.node.id, amount);
        }
        contentItem: Column {
            spacing: 8
            Label { text: qsTr("Monete nella nuova pila") }
            SpinBox {
                id: splitQuantity
                objectName: "inventorySplitQuantity"
                editable: true
                from: 1
                to: Math.max(1, (root.node.quantity ?? 1) - 1)
                Accessible.name: qsTr("Monete nella nuova pila")
            }
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: splitPicker.reject() }
            Button { objectName: "inventoryConfirmSplit"; text: qsTr("Dividi"); onClicked: splitPicker.submit() }
        }
    }
    HoverHandler { id: cardHover }
    component CapacityRing: Item {
        id: capacity
        property bool measuresVolume: false
        readonly property var container: root.node.container
        readonly property var current: measuresVolume ? container?.currentContentsVolumeMilliliters : container?.currentContentsWeightGrams
        readonly property var maximum: measuresVolume ? container?.maximumContentsVolumeMilliliters : container?.maximumContentsWeightGrams
        function format(value) {
            return measuresVolume ? root.panel.volume(value) : root.panel.weight(value);
        }
        objectName: (measuresVolume ? "inventoryVolumeCapacity_" : "inventoryCapacity_") + root.node.id
        visible: !!container && (!measuresVolume || (current != null && !unlimited))
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48
        readonly property real fillAmount: maximum > 0 ? Math.min(1, Math.max(0, current / maximum)) : 0
        readonly property bool dashed: measuresVolume ? container?.ignoresContentsVolume === true : !!root.panel.weightExclusionText(root.node)
        readonly property bool unlimited: maximum == null
        readonly property string tooltipText: measuresVolume ? qsTr("Volume: %1 / %2 — %3%").arg(format(current)).arg(format(maximum)).arg(Math.round(fillAmount * 100))
            + (dashed ? "\n" + qsTr("Il contenuto non aggiunge volume all’esterno.") : "")
            : root.panel.capacityText(root.node.container)
                + (dashed ? "\n" + root.panel.weightExclusionText(root.node) : "")
        Accessible.name: tooltipText
        ToolTip.visible: capacityHover.hovered
        ToolTip.text: tooltipText
        HoverHandler { id: capacityHover }
        Image {
            objectName: "dashedCapacityRing"
            anchors.fill: parent
            visible: capacity.dashed
            sourceSize: Qt.size(96, 96)
            source: {
                if (!capacity.dashed) return "";
                const circumference = 42 * Math.PI;
                const count = 16;
                const step = circumference / count;
                const dash = step * 0.6;
                const gap = step - dash;
                const filled = Math.floor(capacity.fillAmount * count);
                function circle(color, pattern) {
                    return '<circle cx="24" cy="24" r="21" fill="none" stroke="' + color
                        + '" stroke-width="3" stroke-dasharray="' + pattern
                        + '" transform="rotate(-90 24 24)"/>';
                }
                const track = circle("#aebfca", dash + " " + gap);
                const progress = filled ? circle(capacity.measuresVolume ? "#397c70" : "#52768e",
                    (dash + " " + gap + " ").repeat(filled - 1) + dash + " "
                    + (circumference - (filled - 1) * step - dash)) : "";
                const svg = '<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 48 48">'
                    + track + progress + "</svg>";
                return "data:image/svg+xml," + encodeURIComponent(svg);
            }
        }
        Canvas {
            id: capacityCanvas
            anchors.fill: parent
            visible: !capacity.dashed
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Connections {
                target: capacity
                function onFillAmountChanged() { capacityCanvas.requestPaint(); }
            }
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                const x = width / 2, y = height / 2, radius = Math.min(width, height) / 2 - 3;
                ctx.lineWidth = 3;
                ctx.lineCap = "round";
                function strokeArc(fraction, color) {
                    ctx.beginPath();
                    ctx.arc(x, y, radius, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * fraction);
                    ctx.strokeStyle = color;
                    ctx.stroke();
                }
                strokeArc(1, "#d8e2e8");
                if (capacity.fillAmount > 0) strokeArc(capacity.fillAmount, capacity.measuresVolume ? "#397c70" : "#52768e");
            }
        }
        Label {
            z: 1
            objectName: (capacity.measuresVolume ? "inventoryContentsVolume_" : "inventoryContentsWeight_") + root.node.id
            anchors.centerIn: parent
            width: 36; height: 32
            text: capacity.format(capacity.current).replace(" ", "\n")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 11
            fontSizeMode: Text.Fit
            minimumPixelSize: 8
            color: "#253947"
        }
    }
    DropArea {
        id: destination
        objectName: "inventoryDrop_" + root.node.id
        anchors.fill: root
        z: -1
        enabled: !!root.node.container || root.panel.isCoinStack(root.panel.item(root.node.id))
        keys: ["inventory-item", "inventory-unequip"]
        onEntered: drag => {
            const itemId = root.draggedItemId(drag.source);
            const unequip = root.fromEquipment(drag.source);
            drag.accepted = root.node.container
                ? root.panel.canRequestMove(itemId, root.node.container.id, unequip)
                : !unequip && root.panel.canMergeCoinStacks(itemId, root.node.id);
        }
        onDropped: drop => {
            const itemId = root.draggedItemId(drop.source);
            if (!root.node.container) {
                if (root.fromEquipment(drop.source) || !root.panel.canMergeCoinStacks(itemId, root.node.id)) return;
                drop.acceptProposedAction();
                root.panel.mergeCoinStacksRequested(itemId, root.node.id);
                return;
            }
            const containerId = root.node.container.id;
            const unequip = root.fromEquipment(drop.source);
            if (!root.panel.canRequestMove(itemId, containerId, unequip)) return;
            drop.acceptProposedAction();
            if (unequip) root.panel.unequipItemRequested(itemId, containerId);
            else root.panel.moveItemRequested(itemId, containerId);
        }
    }
    Repeater {
        model: root.node.rootContainer ? [] : [false, true]
        DropArea {
            id: insertion
            required property bool modelData
            objectName: (modelData ? "inventoryReorderAfter_" : "inventoryReorderBefore_") + root.node.id
            x: 0
            readonly property real edgeDepth: Math.min(9, root.height / 4)
            readonly property real gapInset: 3
            y: modelData ? root.height - edgeDepth : -gapInset
            width: root.width
            height: edgeDepth + gapInset
            z: 2
            keys: ["inventory-item"]
            onEntered: drag => {
                drag.accepted = root.panel.reorderDestination(root.draggedItemId(drag.source), root.node.id, modelData) !== undefined;
            }
            onDropped: drop => {
                const itemId = root.draggedItemId(drop.source);
                const placement = root.panel.reorderDestination(itemId, root.node.id, modelData);
                if (!placement) return;
                drop.acceptProposedAction();
                if (placement.moving)
                    root.panel.moveAndReorderItemRequested(itemId, placement.containerId, placement.beforeItemId);
                else
                    root.panel.reorderItemRequested(itemId, placement.beforeItemId);
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 3
                y: (insertion.modelData ? parent.height : 0) - height / 2
                height: 3
                radius: 1
                color: "#1769aa"
                visible: insertion.containsDrag
            }
        }
    }
    Column {
        id: body
        x: 8; y: 8
        width: parent.width - 16
        spacing: 8
        Item {
            id: header
            width: parent.width
            implicitHeight: headerContent.implicitHeight
            RowLayout {
                id: headerContent
                anchors.fill: parent
                spacing: 6
            ToolButton {
                id: disclosure
                objectName: "toggleContainer_" + root.node.id
                opacity: root.node.container ? 1 : 0
                enabled: !!root.node.container
                Accessible.ignored: !root.node.container
                text: root.panel.collapsedContainers.includes(root.node.container?.id) ? "▸" : "▾"
                implicitWidth: 24; implicitHeight: 26
                transform: Translate {
                    y: nameHeader.parent.y + nameHeader.y + nameHeader.height / 2
                        - disclosure.y - disclosure.height / 2
                }
                padding: 0
                background: Item {}
                contentItem: Canvas {
                    id: arrow
                    property bool expanded: disclosure.text === "▾"
                    property color ink: disclosure.hovered || disclosure.visualFocus ? "#365f75" : "#687c88"
                    onExpandedChanged: requestPaint()
                    onInkChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d");
                        ctx.reset();
                        ctx.translate(width / 2, height / 2);
                        if (!arrow.expanded) ctx.rotate(-Math.PI / 2);
                        ctx.beginPath();
                        ctx.moveTo(-7, -4);
                        ctx.lineTo(7, -4);
                        ctx.lineTo(0, 4);
                        ctx.closePath();
                        ctx.fillStyle = arrow.ink;
                        ctx.fill();
                    }
                }
                onClicked: root.panel.toggleContainer(root.node.container.id)
                Accessible.name: qsTr("Apri o chiudi %1").arg(root.node.name)
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 2
                Item {
                    id: nameHeader
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    property bool editingQuantity: false
                    readonly property bool quantityEditable: !root.node.rootContainer
                        && (root.panel.item(root.node.id)?.canStack ?? (root.node.quantity > 1))
                    readonly property int quantitySpace: quantityEditable ? (editingQuantity ? quantityInput.width : quantityButton.width) + 4 : 0
                    implicitWidth: nameLabel.implicitWidth + quantitySpace + (info.visible ? 26 : 0)
                    implicitHeight: Math.max(quantityEditable ? 58 : 22, nameLabel.implicitHeight)
                    DragHandler {
                        id: itemDrag
                        target: null
                        enabled: !root.node.rootContainer && !root.node.equipped && !nameHeader.editingQuantity
                        onActiveChanged: {
                            if (active) {
                                root.panel.draggedItemId = root.node.id;
                                dragMarker.Drag.active = true;
                            } else {
                                root.panel.draggedItemId = "";
                                if (dragMarker.Drag.active) dragMarker.Drag.drop();
                            }
                        }
                        onCanceled: dragMarker.Drag.cancel()
                    }
                    Item {
                        id: dragMarker
                        readonly property string itemId: root.node.id
                        x: itemDrag.centroid.position.x
                        y: itemDrag.centroid.position.y
                        Drag.keys: ["inventory-item"]
                        Drag.supportedActions: Qt.MoveAction
                        Drag.proposedAction: Qt.MoveAction
                    }
                    Rectangle {
                        id: dragPreview
                        objectName: "inventoryDragPreview_" + root.node.id
                        parent: Overlay.overlay
                        visible: itemDrag.active
                        z: 1
                        readonly property point pointer: visible && parent
                            ? nameHeader.mapToItem(parent, itemDrag.centroid.position.x, itemDrag.centroid.position.y)
                            : Qt.point(0, 0)
                        x: pointer.x + 12
                        y: pointer.y + 10
                        width: Math.min(240, previewName.implicitWidth + 20)
                        height: previewName.implicitHeight + 12
                        radius: 5
                        color: "#fafbfc"
                        border.color: "#52768e"
                        Label {
                            id: previewName
                            anchors.fill: parent
                            anchors.margins: 6
                            text: nameLabel.text
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.pixelSize: 15
                            color: "#253947"
                        }
                    }
                    Label {
                        id: nameLabel
                        objectName: "inventoryName_" + root.node.id
                        width: Math.min(implicitWidth, Math.max(0, nameHeader.width - nameHeader.quantitySpace - (info.visible ? 26 : 0)))
                        y: (nameHeader.height - height) / 2
                        text: root.node.name
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        font.bold: !!root.node.rootContainer
                        color: "#253947"
                    }
                    ToolButton {
                        id: quantityButton
                        objectName: "inventoryQuantity_" + root.node.id
                        opacity: nameHeader.editingQuantity ? 0 : 1
                        enabled: !nameHeader.editingQuantity
                        visible: nameHeader.quantityEditable && (root.node.quantity > 1 || (cardHover.hovered && !contentsHover.hovered) || nameHeader.editingQuantity)
                        x: nameLabel.width + 4
                        y: (nameHeader.height - height) / 2
                        height: 22
                        width: implicitWidth
                        padding: 0
                        text: "×" + root.node.quantity
                        Accessible.name: qsTr("Modifica quantità di %1").arg(root.node.name)
                        background: Item {}
                        contentItem: Label { text: quantityButton.text; font.pixelSize: 15; color: quantityButton.hovered || quantityButton.visualFocus ? "#1769aa" : "#253947" }
                        onClicked: {
                            quantityInput.text = String(root.node.quantity);
                            nameHeader.editingQuantity = true;
                            quantityInput.forceActiveFocus();
                            quantityInput.selectAll();
                        }
                    }
                    Item {
                        id: quantityArrows
                        x: quantityButton.x
                        y: (nameHeader.height - height) / 2
                        width: quantityButton.width
                        height: 58
                        visible: quantityButton.visible && !nameHeader.editingQuantity
                        HoverHandler { id: quantityHover }
                        Repeater {
                            model: [1, -1]
                            ToolButton {
                                id: stepButton
                                required property int modelData
                                objectName: modelData > 0 ? "inventoryQuantityIncrease" : "inventoryQuantityDecrease"
                                width: quantityArrows.width
                                height: 18
                                y: modelData > 0 ? 0 : quantityArrows.height - height
                                padding: 0
                                visible: quantityHover.hovered || visualFocus
                                enabled: modelData > 0 ? root.node.quantity < 2147483647 : root.node.quantity > 1
                                text: modelData > 0 ? "▴" : "▾"
                                Accessible.name: modelData > 0 ? qsTr("Aumenta quantità") : qsTr("Diminuisci quantità")
                                background: Item {}
                                contentItem: Label {
                                    text: stepButton.text
                                    font.pixelSize: 22
                                    color: stepButton.hovered ? "#1769aa" : "#536976"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: root.panel.itemQuantityChangeRequested(root.node.id, root.node.quantity + modelData)
                            }
                        }
                    }
                    TextField {
                        id: quantityInput
                        objectName: "inventoryQuantityInput"
                        visible: nameHeader.editingQuantity
                        x: quantityButton.x
                        y: (nameHeader.height - height) / 2
                        width: 70
                        height: 24
                        padding: 3
                        Accessible.name: qsTr("Nuova quantità")
                        validator: RegularExpressionValidator { regularExpression: /[0-9]{1,10}/ }
                        onAccepted: {
                            if (!acceptableInput) return;
                            const quantity = Number(text);
                            if (quantity < 1 || quantity > 2147483647) return;
                            nameHeader.editingQuantity = false;
                            if (quantity !== root.node.quantity) root.panel.itemQuantityChangeRequested(root.node.id, quantity);
                        }
                        onActiveFocusChanged: if (!activeFocus) nameHeader.editingQuantity = false
                        Keys.onEscapePressed: nameHeader.editingQuantity = false
                    }
                    InfoButton {
                        id: info
                        objectName: "inventoryInfo_" + root.node.id
                        visible: !root.node.rootContainer
                        checked: root.panel.expandedItemDetails.includes(root.node.id)
                        onToggled: root.panel.setItemDetailsExpanded(root.node.id, checked)
                        opacity: (cardHover.hovered && !contentsHover.hovered) || visualFocus ? 1 : 0
                        width: 22; height: 22
                        x: nameLabel.width + nameHeader.quantitySpace + 4
                        y: (nameHeader.height - height) / 2
                        Accessible.name: qsTr("Informazioni su %1").arg(root.node.name)
                    }
                }
                Label { visible: root.node.equipped === true; text: qsTr("Equipaggiato"); font.pixelSize: 11; color: "#52768e" }
            }
            Label { objectName: "inventoryWeight_" + root.node.id; visible: !root.node.rootContainer; text: root.panel.rowWeight(root.node); font.pixelSize: 12; color: "#687c88" }
            CapacityRing {}
            CapacityRing { measuresVolume: true }
        }
        }
        InfoDetails {
            objectName: "inventoryDetails_" + root.node.id
            visible: info.checked
            width: parent.width
            rows: {
                if (!visible) return [];
                const details = root.panel.itemInfoRows(root.node.id);
                const container = root.node.container;
                if (container?.currentContentsVolumeMilliliters != null && container.maximumContentsVolumeMilliliters == null)
                    details.push({kind: InfoDetails.Entry, text: qsTr("Volume del contenuto: %1 (senza limite)").arg(root.panel.volume(container.currentContentsVolumeMilliliters))});
                return details;
            }
        }
        Repeater {
            model: info.checked ? root.panel.item(root.node.id)?.usagePools ?? [] : []
            RowLayout {
                id: usageRow
                required property var modelData
                objectName: "inventoryUsagePool_" + root.node.id + "_" + modelData.id
                width: parent.width
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Cariche (%1): %2 / %3").arg(usageRow.modelData.id)
                        .arg(usageRow.modelData.remaining).arg(usageRow.modelData.capacity)
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    objectName: "inventoryUsagePoolBalance"
                }
                ToolButton {
                    objectName: "inventoryRestoreUsage"
                    text: "+"
                    enabled: usageRow.modelData.remaining < usageRow.modelData.capacity
                    Accessible.name: qsTr("Aggiungi una carica")
                    onClicked: root.panel.restoreItemUsagePoolRequested(root.node.id, usageRow.modelData.id, 1)
                }
                Button {
                    objectName: "inventoryRefillUsage"
                    text: qsTr("Ricarica")
                    enabled: usageRow.modelData.remaining < usageRow.modelData.capacity
                    onClicked: root.panel.refillItemUsagePoolRequested(root.node.id, usageRow.modelData.id)
                }
            }
        }
        Button {
            objectName: "inventorySplit_" + root.node.id
            readonly property alias editor: splitPicker
            visible: info.checked && root.canSplitCoins
            text: qsTr("Dividi")
            onClicked: splitPicker.open()
        }
        Column {
            id: childCards
            width: parent.width
            spacing: 6
            visible: !!root.node.children?.length
            HoverHandler { id: contentsHover; enabled: childCards.visible }
            Repeater {
                model: root.node.children ?? []
                Loader {
                    id: child
                    required property var modelData
                    width: childCards.width
                    Component.onCompleted: setSource(Qt.resolvedUrl("InventoryCard.qml"), {node: modelData, panel: root.panel})
                }
            }
        }
    }
}
