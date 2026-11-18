import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root
    required property int sides
    required property int result
    required property int classLevel
    implicitWidth: 42
    implicitHeight: 44
    Accessible.name: qsTr("Livello %1, d%2: %3").arg(classLevel).arg(sides).arg(result || qsTr("da inserire"))
    ToolTip.visible: hovered
    ToolTip.text: Accessible.name
    background: Image {
        objectName: "hitDieSvg"
        source: root.sides === 6 ? "die-d6.svg" : root.sides === 8 ? "die-d8.svg" : root.sides === 10 ? "die-d10.svg" : "die-d12.svg"
        sourceSize.width: 84
        sourceSize.height: 88
        opacity: root.hovered || root.result ? 1 : 0.65
    }
    contentItem: Label {
        text: root.result > 0 ? root.result.toString() : "+"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        rightPadding: root.sides === 6 ? 18 : 0
        topPadding: root.sides === 6 ? 10 : root.sides === 8 ? 2 : root.sides === 12 ? 4 : 8
        font.bold: true
        font.pixelSize: 14
        color: "#253947"
    }
}
