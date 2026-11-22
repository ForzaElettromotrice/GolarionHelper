pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var hitPoints
    property bool readOnly: false
    property real maximumDetailsWidth: 320
    property Component headerExtension
    property Component belowHeader
    property bool editing: false
    readonly property int hitPointRowHeight: 54
    readonly property real detailsWidth: Math.min(maximumDetailsWidth, Math.max(0, width - 32))
    readonly property real hitPointFraction: hitPoints?.max > 0 ? Math.max(0, Math.min(1, hitPoints.current / hitPoints.max)) : 0
    readonly property real healthBarWidth: Math.max(0, detailsWidth - Math.ceil(hitPointCaption.implicitWidth) - 6)
    readonly property real temporaryWidth: {
        const total = hitPoints?.temporary?.total ?? 0;
        if (total <= 0) return 0;
        const maximum = Math.floor(healthBarWidth * 0.45);
        const minimum = Math.min(64, maximum);
        const maxHitPoints = hitPoints?.max ?? 0;
        const proportional = maxHitPoints > 0 ? Math.round(healthBarWidth * total / (maxHitPoints + total)) : maximum;
        return Math.max(minimum, Math.min(maximum, proportional));
    }
    function cancelEdit() { editing = false; amount.clear(); amount.focus = false; }
    function finishFocusChange() {
        Qt.callLater(() => {
            if (editing && !amount.activeFocus && !nonLethal.activeFocus) cancelEdit();
        });
    }
    enum DamageLethality { Lethal, NonLethal }
    signal damageRequested(int amount, int lethality)
    signal healRequested(int amount)
    signal temporaryAddRequested(string sourceId, int amount)
    signal temporaryRemoveRequested(string sourceId)
    implicitWidth: 360
    implicitHeight: content.height

    MouseArea {
        parent: root.Window.window ? root.Window.window.contentItem : null
        anchors.fill: parent
        z: 100000
        enabled: root.editing && root.visible
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onPressed: mouse => {
            const inEditor = amount.contains(amount.mapFromItem(parent, mouse.x, mouse.y));
            const inCheckbox = nonLethal.contains(nonLethal.mapFromItem(parent, mouse.x, mouse.y));
            mouse.accepted = false;
            if (!inEditor && !inCheckbox) root.cancelEdit();
        }
    }

    function positiveAmount(text) {
        return /^\d+$/.test(text) && Number(text) > 0 && Number(text) <= 2147483647;
    }
    function maximumRows() {
        const rows = [{kind: InfoDetails.Heading, text: qsTr("PF massimi")}];
        if (!hitPoints) return [{kind: InfoDetails.Note, text: qsTr("Dati non disponibili.")}];
        for (const entry of hitPoints.maxContributions?.contributions ?? [])
            rows.push({kind: InfoDetails.Entry, text: (entry.resolvedValue >= 0 ? "+" : "") + entry.resolvedValue + " " + entry.id});
        rows.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + hitPoints.max});
        return rows;
    }

    Popup {
        id: temporaryPopup
        objectName: "temporaryPopup"
        parent: addTemporary
        x: Math.max(0, Math.min(addTemporary.mapToItem(root, 0, 0).x, root.width - width)) - addTemporary.mapToItem(root, 0, 0).x
        y: addTemporary.height + 6
        width: Math.min(280, root.width)
        padding: 14
        margins: 8
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        onOpened: { source.clear(); temporaryAmount.clear(); source.forceActiveFocus(); }
        function submit() {
            if (!root.hitPoints || !source.text.trim() || !root.positiveAmount(temporaryAmount.text)) return;
            const sourceId = source.text.trim();
            const value = Number(temporaryAmount.text);
            close();
            root.temporaryAddRequested(sourceId, value);
        }
        background: Rectangle { color: "#f8fafb"; border.color: "#aab9c3"; radius: 6 }
        contentItem: Column {
            spacing: 10
            Label { text: qsTr("Aggiungi PF temporanei"); font.bold: true; color: "#213e50" }
            TextField {
                id: source
                objectName: "temporarySource"
                ContextMenu.menu: null
                width: parent.width
                placeholderText: qsTr("Fonte")
                Accessible.name: placeholderText
                onAccepted: temporaryPopup.submit()
            }
            TextField {
                id: temporaryAmount
                objectName: "temporaryAmount"
                ContextMenu.menu: null
                width: parent.width
                placeholderText: qsTr("Quantità")
                inputMethodHints: Qt.ImhDigitsOnly
                Accessible.name: qsTr("Quantità di PF temporanei")
                onAccepted: temporaryPopup.submit()
            }
            Button {
                objectName: "confirmTemporary"
                width: parent.width
                text: qsTr("Aggiungi")
                enabled: root.hitPoints != null && source.text.trim().length > 0 && root.positiveAmount(temporaryAmount.text)
                onClicked: temporaryPopup.submit()
            }
        }
    }

    Column {
        id: content
        width: root.width
        spacing: 6
        Flow {
            width: parent.width
            spacing: 12
        Column {
            width: root.detailsWidth + 32
            spacing: 2
        RowLayout {
            width: parent.width
            spacing: 6
            Label {
                id: hitPointCaption
                Layout.preferredWidth: Math.ceil(implicitWidth)
                Layout.minimumWidth: Layout.preferredWidth
                Layout.maximumWidth: Layout.preferredWidth
                text: qsTr("PF")
                objectName: "hitPointCaption"
                Layout.preferredHeight: root.hitPointRowHeight
                Layout.minimumHeight: root.hitPointRowHeight
                Layout.maximumHeight: root.hitPointRowHeight
                color: "white"
                font.bold: true
                font.pixelSize: 20
                padding: 14
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { color: "#181818" }
            }
            Item {
                Layout.preferredWidth: root.healthBarWidth - root.temporaryWidth
                Layout.minimumWidth: Layout.preferredWidth
                Layout.maximumWidth: Layout.preferredWidth
                Layout.preferredHeight: root.hitPointRowHeight
                Layout.minimumHeight: root.hitPointRowHeight
                Layout.maximumHeight: root.hitPointRowHeight
            Label {
                id: currentLabel
                objectName: "currentAndMaximum"
                anchors.fill: parent
                visible: !root.editing
                text: (root.hitPoints?.current ?? "—") + " / " + (root.hitPoints?.max ?? "—")
                font.pixelSize: 24
                color: "#181818"
                padding: 10
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                background: Rectangle {
                    color: "#f5f5f5"
                    Rectangle {
                        objectName: "hitPointFill"
                        width: parent.width * root.hitPointFraction
                        height: parent.height
                        color: "#b9d9b0"
                    }
                    Rectangle { anchors.fill: parent; color: "transparent"; border.color: "#777777" }
                }
                Accessible.name: qsTr("PF attuali / massimi")
                TapHandler {
                    enabled: root.hitPoints != null && !root.readOnly
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onTapped: (eventPoint, button) => {
                        root.editing = true;
                        amount.clear();
                        nonLethal.checked = button === Qt.RightButton;
                        amount.forceActiveFocus();
                    }
                }
            }
            TextField {
                ContextMenu.menu: null
                id: amount
                readonly property bool handlesOutsideClicks: true
                objectName: "amount"
                anchors.fill: parent
                visible: root.editing
                enabled: root.hitPoints != null
                placeholderText: "+ / −"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
                color: "#181818"
                background: Rectangle { color: "#fff9e9"; border.color: "#777777" }
                Accessible.name: qsTr("Variazione PF: più per cura, meno per danni. Invio per confermare.")
                onActiveFocusChanged: root.finishFocusChange()
                Keys.onEscapePressed: root.cancelEdit()
                onAccepted: {
                    const command = text.trim();
                    if (!/^[+-]\d+$/.test(command) || !root.positiveAmount(command.slice(1))) return;
                    const value = Number(command.slice(1));
                    const lethality = nonLethal.checked ? HitPointsPanel.NonLethal : HitPointsPanel.Lethal;
                    root.cancelEdit();
                    if (command[0] === "+") root.healRequested(value);
                    else root.damageRequested(value, lethality);
                }
            }
            }
            Rectangle {
                objectName: "temporaryExtension"
                visible: root.temporaryWidth > 0
                Layout.preferredWidth: root.temporaryWidth
                Layout.minimumWidth: root.temporaryWidth
                Layout.preferredHeight: root.hitPointRowHeight
                Layout.minimumHeight: root.hitPointRowHeight
                Layout.maximumHeight: root.hitPointRowHeight
                Layout.leftMargin: -6
                color: "#c3e1f2"
                border.color: "#657f90"
                Column {
                    anchors.centerIn: parent
                    width: parent.width
                    Label { width: parent.width; text: qsTr("TEMP"); font.pixelSize: 10; color: "#365970"; horizontalAlignment: Text.AlignHCenter }
                    Label {
                        objectName: "temporaryTotal"
                        width: parent.width
                        text: "+" + (root.hitPoints?.temporary?.total ?? 0)
                        font.pixelSize: 18
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 10
                        color: "#213e50"
                        horizontalAlignment: Text.AlignHCenter
                        Accessible.name: qsTr("PF temporanei")
                    }
                }
            }
            Column {
                spacing: 2
            ToolButton {
                id: addTemporary
                objectName: "addTemporary"
                visible: !root.readOnly
                width: 26
                height: 26
                text: "+"
                onClicked: temporaryPopup.visible ? temporaryPopup.close() : temporaryPopup.open()
                Accessible.name: qsTr("Aggiungi PF temporanei")
                contentItem: Label {
                    text: addTemporary.text
                    font.pixelSize: 20
                    color: "#365970"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: width / 2
                    color: addTemporary.hovered || addTemporary.visualFocus ? "#dcecf6" : "#f5f9fc"
                    border.color: "#657f90"
                }
            }
            InfoButton {
                id: maximumInfo
                objectName: "maximumInfo"
                width: 26
                height: 26
                Accessible.name: qsTr("Dettagli PF massimi e temporanei")
            }
            }
        }
        Label {
            objectName: "nonLethalSummary"
            text: qsTr("Danni non letali: ") + (root.hitPoints?.nonLethal ?? "—")
            color: "#555555"
        }
        }
        Loader {
            sourceComponent: root.headerExtension
            width: Math.min(content.width, (item as Item)?.implicitWidth ?? 0)
        }
        }
        CheckBox {
            id: nonLethal
            objectName: "nonLethal"
            visible: root.editing
            text: qsTr("Non letali")
            onActiveFocusChanged: root.finishFocusChange()
            onClicked: amount.forceActiveFocus()
            Keys.onEscapePressed: root.cancelEdit()
        }
        InfoFrame {
            id: detailsFrame
            objectName: "hitPointDetailsFrame"
            width: root.detailsWidth
            height: detailsContent.height + 8
            visible: maximumInfo.checked
        Column {
            id: detailsContent
            width: parent.width
        InfoDetails { objectName: "maximumDetails"; framed: false; width: parent.width; rows: root.maximumRows() }
        Column {
            objectName: "temporaryDetails"
            width: parent.width
            visible: maximumInfo.checked
            spacing: 4
            InfoDetails {
                framed: false
                width: parent.width
                topPadding: 4
                bottomPadding: 0
                rows: [{kind: InfoDetails.Heading, text: qsTr("Riserve temporanee")}]
            }
            Repeater {
                model: root.hitPoints?.temporary?.pools ?? []
                Rectangle {
                    id: poolRow
                    required property var modelData
                    x: 14
                    width: detailsFrame.width - 28
                    implicitHeight: Math.max(58, poolContent.implicitHeight + 16)
                    color: "#f2f5f7"
                    radius: 4
                    RowLayout {
                        id: poolContent
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 12
                        Column {
                            Layout.minimumWidth: 40
                            Label {
                                width: parent.width
                                text: poolRow.modelData.remaining
                                color: "#344d60"
                                font.pixelSize: 22
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label { width: parent.width; text: qsTr("PF"); color: "#657786"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                        }
                        Label {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            text: poolRow.modelData.id
                            color: "#181818"
                            font.pixelSize: 14
                            textFormat: Text.PlainText
                            wrapMode: Text.WrapAnywhere
                        }
                        ToolButton {
                            id: removePool
                            objectName: "removeTemporary"
                            visible: !root.readOnly
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 28
                            text: "×"
                            readonly property string sourceId: poolRow.modelData.id
                            Accessible.name: qsTr("Rimuovi riserva ") + sourceId
                            ToolTip.visible: hovered
                            ToolTip.text: Accessible.name
                            background: Item {}
                            contentItem: Label {
                                text: removePool.text
                                color: removePool.hovered || removePool.visualFocus ? "#aa3333" : "#657786"
                                font.pixelSize: 20
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: root.temporaryRemoveRequested(sourceId)
                        }
                    }
                }
            }
            Label {
                width: parent.width
                padding: 14
                visible: !(root.hitPoints?.temporary?.pools?.length)
                text: qsTr("Nessuna riserva temporanea.")
                color: "#666666"
                font.italic: true
                wrapMode: Text.Wrap
            }
        }
        }
        }
        Loader {
            width: parent.width
            sourceComponent: root.belowHeader
        }
    }
}
