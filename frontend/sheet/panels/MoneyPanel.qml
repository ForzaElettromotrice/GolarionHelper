pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum Denomination { Copper, Silver, Gold, Platinum }
    required property var money
    property var containers: []
    signal addMoneyRequested(int denomination, int quantity, string containerId)
    signal convertMoneyRequested(int fromDenomination, int toDenomination, int fromQuantity, int toQuantity, string sourceContainerId, string destinationContainerId)
    property string selectedSourceId: ""
    property string selectedDestinationId: ""
    readonly property var sourceAmounts: (money?.containers ?? []).find(entry => entry.containerId === source.currentValue)?.amounts
    readonly property var denominations: [
        {type: MoneyPanel.Copper, key: "copper", name: qsTr("Rame"), color: "#80543e", fill: "#f4e7dd"},
        {type: MoneyPanel.Silver, key: "silver", name: qsTr("Argento"), color: "#536976", fill: "#e9eef1"},
        {type: MoneyPanel.Gold, key: "gold", name: qsTr("Oro"), color: "#7a5a1f", fill: "#faf0d5"},
        {type: MoneyPanel.Platinum, key: "platinum", name: qsTr("Platino"), color: "#465c68", fill: "#e2ecef"}
    ]
    spacing: 12

    Label { text: qsTr("Monete"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    RowLayout {
        width: parent.width
        Label { text: qsTr("Da:") }
        ComboBox {
            id: source
            objectName: "moneySource"
            Layout.fillWidth: true
            model: root.containers
            textRole: "name"
            valueRole: "id"
            currentIndex: {
                const selected = root.containers.findIndex(container => container.id === root.selectedSourceId);
                if (selected >= 0) return selected;
                const worn = root.containers.findIndex(container => container.id === "worn");
                return worn >= 0 ? worn : (root.containers.length ? 0 : -1);
            }
            onActivated: root.selectedSourceId = currentValue
            Accessible.name: qsTr("Contenitore di origine")
        }
        Label { text: qsTr("A:") }
        ComboBox {
            id: destination
            objectName: "moneyDestination"
            Layout.fillWidth: true
            model: root.containers
            textRole: "name"
            valueRole: "id"
            currentIndex: {
                const selected = root.containers.findIndex(container => container.id === root.selectedDestinationId);
                return selected >= 0 ? selected : source.currentIndex;
            }
            onActivated: root.selectedDestinationId = currentValue
            Accessible.name: qsTr("Contenitore di destinazione")
        }
    }
    Item {
        id: coinArea
        width: parent.width
        implicitHeight: coinGrid.implicitHeight
    GridLayout {
        id: coinGrid
        readonly property bool wide: width >= 420
        width: parent.width
        columns: wide ? 4 : 2
        columnSpacing: 8
        rowSpacing: 8
        Repeater {
            id: coinCards
            property int generation: 0
            onItemAdded: ++generation
            onItemRemoved: ++generation
            function cardAt(index) { return generation > 0 ? itemAt(index) : null; }
            model: root.denominations
            Rectangle {
                id: card
                required property var modelData
                objectName: "moneyCard_" + modelData.key
                readonly property var total: root.money?.total?.[modelData.key]
                property bool editing: false
                onTotalChanged: editing = false
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                implicitHeight: contents.height + 20
                radius: 6
                color: modelData.fill
                Column {
                    id: contents
                    anchors.centerIn: parent
                    width: parent.width - 16
                    spacing: 4
                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: card.modelData.name; color: card.modelData.color; font.bold: true }
                    Item {
                        width: parent.width
                        height: 36
                        ToolButton {
                            id: totalButton
                            objectName: "moneyAmount_" + card.modelData.key
                            anchors.fill: parent
                            visible: !card.editing
                            enabled: destination.currentIndex >= 0 && Number.isSafeInteger(Number(card.total)) && card.total != null
                            padding: 0
                            text: typeof card.total === "number" ? card.total.toLocaleString(Qt.locale("it_IT"), "f", 0) : (card.total ?? "—")
                            Accessible.name: qsTr("Modifica totale: %1").arg(card.modelData.name)
                            background: Item {}
                            contentItem: Label {
                                text: totalButton.text
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 22
                                font.bold: true
                                color: "#253947"
                            }
                            onClicked: {
                                quantity.text = String(card.total);
                                card.editing = true;
                                quantity.forceActiveFocus();
                                quantity.selectAll();
                            }
                        }
                        TextField {
                            id: quantity
                            objectName: "moneyAddQuantity_" + card.modelData.key
                            anchors.fill: parent
                            visible: card.editing
                            horizontalAlignment: Text.AlignHCenter
                            Accessible.name: qsTr("Nuovo totale: %1").arg(card.modelData.name)
                            validator: RegularExpressionValidator { regularExpression: /[0-9]{1,16}/ }
                            inputMethodHints: Qt.ImhDigitsOnly
                            onAccepted: {
                                const total = Number(text);
                                const delta = total - Number(card.total);
                                if (!acceptableInput || !Number.isSafeInteger(total) || delta < 0 || delta > 2147483647 || !destination.currentValue) return;
                                card.editing = false;
                                if (delta) root.addMoneyRequested(card.modelData.type, delta, destination.currentValue);
                            }
                            onActiveFocusChanged: if (!activeFocus) card.editing = false
                            Keys.onEscapePressed: card.editing = false
                        }
                    }
                }
            }
        }
    }
        Repeater {
            model: root.denominations.length - 1
            Column {
                id: exchange
                required property int index
                objectName: "moneyExchange_" + index
                readonly property Item firstCard: coinCards.cardAt(index)
                readonly property Item secondCard: coinCards.cardAt(index + 1)
                readonly property bool wraps: !coinGrid.wide && index === 1
                visible: !!firstCard && !!secondCard
                z: 1
                x: !visible ? 0 : (wraps ? coinArea.width / 2 : (firstCard.x + firstCard.width + secondCard.x) / 2) - width / 2
                y: !visible ? 0 : (wraps ? (firstCard.y + firstCard.height + secondCard.y) / 2 : firstCard.y + firstCard.height / 2) - height / 2
                Repeater {
                    model: [true, false]
                    ToolButton {
                        id: arrow
                        required property bool modelData
                        readonly property var from: root.denominations[exchange.index + (modelData ? 0 : 1)]
                        readonly property var to: root.denominations[exchange.index + (modelData ? 1 : 0)]
                        readonly property int cost: modelData ? 10 : 1
                        readonly property int proceeds: modelData ? 1 : 10
                        objectName: "moneyConvert_" + from.key + "_" + to.key
                        width: 44
                        height: 28
                        padding: 0
                        text: exchange.wraps ? (modelData ? "↓" : "↑") : (modelData ? "⟶" : "⟵")
                        background: Item {}
                        contentItem: Label {
                            text: arrow.text
                            font.pixelSize: 30
                            font.bold: true
                            color: !arrow.enabled ? "#9aaab3" : arrow.hovered || arrow.visualFocus ? "#1769aa" : "#365f75"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        enabled: !!source.currentValue && !!destination.currentValue && (root.sourceAmounts?.[from.key] ?? 0) >= cost
                        Accessible.name: qsTr("Converti %1 %2 in %3 %4").arg(cost).arg(from.name).arg(proceeds).arg(to.name)
                        ToolTip.visible: hovered
                        ToolTip.text: Accessible.name
                        onClicked: root.convertMoneyRequested(from.type, to.type, cost, proceeds, source.currentValue, destination.currentValue)
                    }
                }
            }
        }
    }
}
