pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Column {
    id: root
    required property var reminders
    property bool prominentHeading: false
    property var tabForChoice: null
    signal openChoiceRequested(int type)
    spacing: 8

    Label {
        objectName: "sectionHeading"
        width: root.prominentHeading ? root.width : implicitWidth
        height: root.prominentHeading ? 42 : implicitHeight
        text: qsTr("Promemoria")
        font.pixelSize: 20
        font.bold: true
        leftPadding: root.prominentHeading ? 12 : 0
        color: root.prominentHeading ? "white" : "#253947"
        verticalAlignment: Text.AlignVCenter
        background: Rectangle { color: "#181818"; visible: root.prominentHeading }
    }
    Label {
        visible: !(root.reminders?.messages?.length) && !(root.reminders?.choices?.length)
        text: qsTr("Nessun promemoria.")
        color: "#536976"
    }
    Repeater {
        model: root.reminders?.choices ?? []
        ItemDelegate {
            id: choice
            required property var modelData
            required property int index
            readonly property bool navigable: root.tabForChoice !== null && root.tabForChoice(modelData.type) >= 0
            objectName: "choiceReminder_" + index
            width: root.width
            padding: 10
            hoverEnabled: navigable
            focusPolicy: navigable ? Qt.StrongFocus : Qt.NoFocus
            Accessible.role: navigable ? Accessible.Button : Accessible.StaticText
            Accessible.name: text
            text: modelData.remaining > 1 ? qsTr("%1 (%2)").arg(modelData.description).arg(modelData.remaining)
                : modelData.description
            contentItem: Label {
                text: choice.text + (choice.navigable ? " →" : "")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: choice.navigable && (choice.hovered || choice.visualFocus) ? "#1769aa" : "#253947"
            }
            background: InfoFrame {}
            onClicked: if (navigable) root.openChoiceRequested(modelData.type)
        }
    }
    Repeater {
        model: root.reminders?.messages ?? []
        Label {
            required property string modelData
            required property int index
            objectName: "reminderMessage_" + index
            width: root.width
            padding: 10
            text: modelData
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: "#253947"
            background: InfoFrame {}
        }
    }
}
