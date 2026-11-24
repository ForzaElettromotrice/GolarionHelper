pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App

ScrollView {
    id: page

    required property SessionController controller
    required property string localName
    readonly property bool hosting: controller.hostingPort > 0
    readonly property bool portValid: portField.acceptableInput
    signal permissionsRequested(string userId, string displayName)

    function startHosting(): void {
        if (portValid && !hosting && !controller.busy)
            controller.startHosting(Number(portField.text), page.localName)
    }

    padding: 20
    contentWidth: availableWidth

    GridLayout {
        width: page.availableWidth
        columns: width < 540 ? 1 : 2
        columnSpacing: 24
        rowSpacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                visible: !page.hosting

                Label { text: qsTr("Porta") }

                TextField {
                    id: portField
                    Layout.preferredWidth: 90
                    text: "42500"
                    readOnly: page.controller.busy
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: RegularExpressionValidator { regularExpression: /[0-9]{1,5}/ }
                    Accessible.name: qsTr("Porta")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Con 0 il sistema sceglie una porta libera.")
                    onAccepted: page.startHosting()
                }

                Item { Layout.fillWidth: true }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: page.hosting
                spacing: 8

                Label { text: qsTr("IP e porta") }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        visible: !addressSelector.visible
                        text: addressSelector.currentText
                    }
                    ComboBox {
                        id: addressSelector
                        Layout.fillWidth: true
                        visible: count > 1
                        model: page.controller.inviteEndpoints
                        Accessible.name: qsTr("IP e porta")
                    }
                    Button {
                        text: qsTr("Copia")
                        Accessible.name: qsTr("Copia indirizzo")
                        onClicked: page.controller.copyInvitationText(addressSelector.currentText)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.controller.errorMessage
                color: "#a12d32"
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }
        }

        ParticipantsPanel {
            visible: page.hosting
            Layout.preferredWidth: 260
            Layout.fillWidth: page.availableWidth < 540
            Layout.alignment: Qt.AlignTop
            participants: page.controller.participants
            onPermissionsRequested: (userId, displayName) => page.permissionsRequested(userId, displayName)
        }
    }
}
