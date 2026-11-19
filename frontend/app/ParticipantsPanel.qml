pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: panel

    required property var participants
    required property string localName

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
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: row.modelData.isSelf && panel.localName.trim().length > 0 ? panel.localName.trim() : row.modelData.userId
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    ToolTip.visible: truncated && nameHover.hovered
                    ToolTip.text: text

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
