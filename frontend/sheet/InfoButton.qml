import QtQuick
import QtQuick.Controls

ToolButton {
    id: root
    width: 28
    padding: 0
    text: "ⓘ"
    checkable: true
    background: Item {}
    contentItem: Label {
        text: root.text
        color: root.checked || root.hovered || root.visualFocus ? "#1769aa" : "#555555"
        font.bold: root.checked || root.hovered || root.visualFocus
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
