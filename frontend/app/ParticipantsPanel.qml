pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: panel

    required property var participants

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

            delegate: RowLayout {
                id: row

                required property var modelData
                Layout.fillWidth: true

                Label {
                    id: participantName
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: row.modelData.displayName
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
                    text: qsTr("Host")
                    font.weight: Font.DemiBold
                }

                Label {
                    visible: row.modelData.isSelf
                    text: qsTr("Tu")
                }
            }
        }
    }
}
