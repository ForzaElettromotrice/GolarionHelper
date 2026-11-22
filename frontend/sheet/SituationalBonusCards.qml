pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Column {
    id: root
    objectName: "situationalBonuses"
    required property var entries
    readonly property var displayedEntries: entries.filter(entry => (entry.values ?? [entry.value]).some(value => value !== 0))
    visible: displayedEntries.length > 0
    spacing: 4

    function signed(value) { return (value >= 0 ? "+" : "") + value; }

    Repeater {
        model: root.displayedEntries
        delegate: Rectangle {
            id: card
            required property var modelData
            objectName: "situationalBonus"
            width: root.width
            height: Math.max(26, conditionLabel.implicitHeight + 8)
            radius: 4
            color: "#eeeeee"
            border.color: "#b5b5b5"
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("%1: %2").arg(modelData.condition).arg(bonusLabel.text)
            Label {
                id: conditionLabel
                width: bonusLabel.x
                height: parent.height
                leftPadding: 10
                rightPadding: 4
                text: card.modelData.condition
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
                color: "#181818"
            }
            Label {
                id: bonusLabel
                objectName: "situationalBonusValue"
                anchors.right: parent.right
                rightPadding: 10
                height: parent.height
                text: {
                    const values = card.modelData.values ?? [card.modelData.value];
                    const labels = card.modelData.labels;
                    return labels ? values.map((value, index) => value === 0 ? "" : root.signed(value) + " " + labels[index])
                        .filter(Boolean).join(" · ")
                        : values.map(value => value === 0 ? "—" : root.signed(value)).join(" / ");
                }
                font.bold: true
                verticalAlignment: Text.AlignVCenter
                color: "#181818"
            }
        }
    }
}
