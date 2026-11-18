import QtQuick
import QtQuick.Controls

Item {
    id: root
    enum Cost { Standard, Move, FullRound, Swift, Immediate, Free, ReplacesAttack, NotAnAction }
    property int cost: -1
    readonly property string description: [qsTr("Azione standard"), qsTr("Azione di movimento"),
        qsTr("Round completo"), qsTr("Azione veloce"), qsTr("Azione immediata"),
        qsTr("Azione gratuita"), qsTr("Sostituisce un attacco"), qsTr("Non è un’azione")][cost] ?? qsTr("Costo non disponibile")
    implicitWidth: 32
    implicitHeight: 32
    Accessible.role: Accessible.StaticText
    Accessible.name: description
    ToolTip.visible: hover.hovered
    ToolTip.text: description
    HoverHandler { id: hover }
    onCostChanged: drawing.requestPaint()
    Canvas {
        id: drawing
        anchors.fill: parent
        onAvailableChanged: if (available) requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.scale(width / 32, height / 32);
            ctx.strokeStyle = "#334957";
            ctx.fillStyle = "#334957";
            ctx.lineWidth = 1.8;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            function line(points, close, fill) {
                ctx.beginPath();
                ctx.moveTo(points[0][0], points[0][1]);
                for (let i = 1; i < points.length; ++i) ctx.lineTo(points[i][0], points[i][1]);
                if (close) ctx.closePath();
                if (fill) ctx.fill(); else ctx.stroke();
            }
            function sword(angle) {
                ctx.save();
                ctx.translate(16, 16);
                ctx.rotate(angle);
                line([[-2, 4], [-2, -7], [0, -12], [2, -7], [2, 4]], true, false);
                line([[-5, 5], [5, 5]], false, false);
                line([[0, 5], [0, 11]], false, false);
                line([[-2, 11], [2, 11]], false, false);
                ctx.restore();
            }
            function circle() {
                ctx.beginPath(); ctx.arc(16, 16, 12, 0, Math.PI * 2); ctx.stroke();
            }
            switch (root.cost) {
            case ActionCostIcon.Standard: sword(0.55); break;
            case ActionCostIcon.FullRound: sword(-0.65); sword(0.65); break;
            case ActionCostIcon.Move:
                // Filled winged sneaker; transparent cuts keep it legible at small sizes.
                ctx.beginPath();
                ctx.moveTo(5, 2);
                ctx.bezierCurveTo(2, 6, 6, 10, 9, 12);
                ctx.bezierCurveTo(6, 11, 3, 9, 2, 7);
                ctx.bezierCurveTo(1, 12, 5, 14, 8, 15);
                ctx.lineTo(3, 14);
                ctx.bezierCurveTo(4, 18, 8, 18, 10, 20);
                ctx.lineTo(18, 26);
                ctx.bezierCurveTo(21, 28, 27, 28, 30, 27);
                ctx.bezierCurveTo(31, 24, 28, 23, 25, 22);
                ctx.bezierCurveTo(22, 21, 21, 19, 20, 16);
                ctx.lineTo(19, 12);
                ctx.bezierCurveTo(18, 10, 16, 12, 16, 13);
                ctx.bezierCurveTo(12, 14, 12, 12, 12, 10);
                ctx.lineTo(11, 10);
                ctx.bezierCurveTo(13, 15, 10, 18, 10, 16);
                ctx.bezierCurveTo(15, 11, 6, 7, 5, 2);
                ctx.closePath(); ctx.fill();
                ctx.beginPath();
                ctx.moveTo(7, 19);
                ctx.bezierCurveTo(3, 21, 8, 25, 12, 26);
                ctx.bezierCurveTo(16, 27, 16, 30, 23, 30);
                ctx.bezierCurveTo(27, 30, 29, 30, 30, 28);
                ctx.bezierCurveTo(25, 29, 20, 29, 17, 27);
                ctx.closePath(); ctx.fill();
                ctx.save();
                ctx.globalCompositeOperation = "destination-out";
                ctx.lineWidth = 1.3;
                line([[18, 16.5], [21, 15.5]], false, false);
                line([[19, 19], [22, 18]], false, false);
                line([[20.5, 21.5], [23.5, 20.5]], false, false);
                ctx.restore();
                break;
            case ActionCostIcon.Immediate:
            case ActionCostIcon.Swift:
                if (root.cost === ActionCostIcon.Immediate) circle();
                line([[18, 6], [10, 18], [16, 18], [14, 26], [23, 13], [17, 13]], true, true); break;
            case ActionCostIcon.Free: circle(); break;
            case ActionCostIcon.ReplacesAttack:
                sword(0);
                line([[20, 23], [29, 23], [26, 20]], false, false);
                line([[29, 23], [26, 26]], false, false); break;
            case ActionCostIcon.NotAnAction:
                line([[9, 16], [23, 16]], false, false); break;
            default:
                ctx.font = "20px sans-serif"; ctx.fillText("?", 10, 23);
            }
        }
    }
}
