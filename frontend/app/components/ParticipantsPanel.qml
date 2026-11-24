pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: panel

    required property var participants
    signal permissionsRequested(string userId, string displayName)

    padding: 12
    contentItem: ColumnLayout {
        spacing: 8

        Label {
            text: qsTr("Partecipanti")
            font.weight: Font.DemiBold
        }

        Label {
            visible: panel.participants.length === 0
            text: qsTr("In attesa della lista…")
        }

        Repeater {
            model: panel.participants

            delegate: ItemDelegate {
                id: row
                objectName: "participantRow"

                required property var modelData
                Layout.fillWidth: true

                enabled: modelData.canEditPermissions === true
                text: modelData.displayName
                Accessible.name: enabled ? qsTr("Permessi di %1").arg(modelData.displayName) : modelData.displayName
                background: Rectangle {
                    visible: row.enabled
                    radius: 4
                    color: row.hovered || row.down ? row.palette.button : "transparent"
                    border.color: row.visualFocus ? row.palette.highlight : "#e1e5ea"
                }
                onClicked: panel.permissionsRequested(modelData.userId, modelData.displayName)

                contentItem: RowLayout {
                    Label {
                        id: participantName
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        color: panel.palette.windowText
                        text: row.text
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        ToolTip {
                            visible: participantName.truncated && nameHover.hovered
                            contentItem: Label {
                                text: participantName.text
                                textFormat: Text.PlainText
                            }
                        }

                        HoverHandler { id: nameHover }
                    }

                    Label {
                        visible: row.modelData.isHost
                        color: panel.palette.windowText
                        text: qsTr("Host")
                        font.weight: Font.DemiBold
                    }

                    Label {
                        visible: row.modelData.isSelf
                        color: panel.palette.windowText
                        text: qsTr("Tu")
                    }

                    Label {
                        visible: row.enabled
                        text: "›"
                        Accessible.ignored: true
                    }
                }
            }
        }
    }
}
