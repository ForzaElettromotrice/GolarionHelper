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
    color: node.rootContainer ? "#e9eef1" : "#ffffff"
    border.color: "#d2dbe1"
    radius: 5
    HoverHandler { id: cardHover }
    Column {
        id: body
        x: 8; y: 8
        width: parent.width - 16
        spacing: 8
        RowLayout {
            width: parent.width
            spacing: 6
            ToolButton {
                objectName: "toggleContainer_" + root.node.id
                opacity: root.node.container ? 1 : 0
                enabled: !!root.node.container
                Accessible.ignored: !root.node.container
                text: root.panel.collapsedContainers.includes(root.node.container?.id) ? "▸" : "▾"
                implicitWidth: 24; implicitHeight: 26
                onClicked: root.panel.toggleContainer(root.node.container.id)
                Accessible.name: qsTr("Apri o chiudi %1").arg(root.node.name)
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        objectName: "inventoryName_" + root.node.id
                        Layout.preferredWidth: implicitWidth
                        Layout.minimumWidth: 0
                        Layout.maximumWidth: implicitWidth
                        Layout.alignment: Qt.AlignVCenter
                        text: root.node.name + (!root.node.rootContainer && root.node.quantity > 1 ? " ×" + root.node.quantity : "")
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        font.bold: !!root.node.rootContainer
                        color: "#253947"
                    }
                    InfoButton {
                        id: info
                        objectName: "inventoryInfo_" + root.node.id
                        visible: !root.node.rootContainer
                        opacity: (cardHover.hovered && !contentsHover.hovered) || visualFocus ? 1 : 0
                        Layout.alignment: Qt.AlignVCenter
                        Layout.minimumWidth: 22; Layout.maximumWidth: 22
                        Layout.minimumHeight: 22; Layout.maximumHeight: 22
                        Accessible.name: qsTr("Informazioni su %1").arg(root.node.name)
                    }
                    Item { Layout.fillWidth: true }
                }
                Label { visible: root.node.equipped === true; text: qsTr("Equipaggiato"); font.pixelSize: 11; color: "#52768e" }
            }
            Label { objectName: "inventoryWeight_" + root.node.id; text: root.panel.rowWeight(root.node); font.pixelSize: 12; color: "#687c88" }
        }
        InfoDetails {
            objectName: "inventoryDetails_" + root.node.id
            visible: info.checked
            width: parent.width
            rows: visible ? root.panel.itemInfoRows(root.node.id) : []
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
