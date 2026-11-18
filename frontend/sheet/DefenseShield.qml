pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property string title
    required property string value
    property string subtitle: ""
    property bool inlineValue: false
    property color fillColor: "#edf1ee"
    property color accentColor: "#425649"
    onFillColorChanged: shieldCanvas.requestPaint()
    onAccentColorChanged: shieldCanvas.requestPaint()
    implicitWidth: 100
    implicitHeight: Math.max(inlineValue ? 70 : 90, labels.implicitHeight + 34)
    Canvas {
        id: shieldCanvas
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.beginPath();
            ctx.moveTo(2, 2);
            ctx.lineTo(width - 2, 2);
            ctx.lineTo(width - 2, height - 30);
            ctx.quadraticCurveTo(width - 2, height - 15, width / 2, height - 2);
            ctx.quadraticCurveTo(2, height - 15, 2, height - 30);
            ctx.closePath();
            ctx.fillStyle = root.fillColor;
            ctx.fill();
            ctx.strokeStyle = root.accentColor;
            ctx.lineWidth = 1.5;
            ctx.stroke();
        }
    }
    Column {
        id: labels
        x: 8; y: 10
        width: parent.width - 16
        spacing: 4
        Label {
            width: parent.width
            text: root.inlineValue ? root.title + " " + root.value : root.title
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: root.inlineValue ? 18 : 13
            font.bold: root.inlineValue
            color: root.accentColor
        }
        Label {
            width: parent.width
            text: root.value
            visible: !root.inlineValue
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 25
            font.bold: true
            color: "#233b2c"
        }
        Label {
            visible: root.subtitle.length > 0
            width: parent.width
            text: root.subtitle
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
            color: root.accentColor
        }
    }
}
