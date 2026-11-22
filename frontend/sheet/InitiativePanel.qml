pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root
    required property var initiative
    implicitWidth: 260
    implicitHeight: content.height

    function signed(value) { return value === undefined ? "—" : (value >= 0 ? "+" : "") + value; }
    readonly property var options: initiative?.abilityOptions ?? []
    readonly property var detailRows: {
        if (!options.length) return [{kind: InfoDetails.Note, text: qsTr("Dati non disponibili.")}];
        const rows = [{kind: InfoDetails.Heading, text: qsTr("Iniziativa")}];
        const abilities = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        for (const option of options) {
            if (options.length > 1) rows.push({kind: InfoDetails.Group, text: option.source});
            rows.push({kind: InfoDetails.Entry, indent: 2, text: signed(option.abilityModifier) + " " + abilities[option.abilityType]});
            rows.push(...ModifierText.rows(option.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            rows.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + signed(option.totalValue)});
        }
        rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return rows;
    }

    Column {
        id: content
        width: Math.min(root.width, root.implicitWidth)
        spacing: 4
        Row {
            width: parent.width
            height: Math.max(48, score.implicitHeight + 16)
            Label {
                width: content.width - 108
                height: parent.height
                text: qsTr("Iniziativa")
                leftPadding: 14
                rightPadding: 12
                font.pixelSize: 16
                font.bold: true
                color: "white"
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { color: "#181818" }
            }
            Label {
                id: score
                objectName: "initiativeScore"
                width: 80
                height: parent.height
                leftPadding: 6
                text: root.options.length ? [...new Set(root.options.map(option => option.totalValue))].map(value => root.signed(value)).join(" / ") : "—"
                font.pixelSize: 20
                color: "#181818"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Accessible.name: qsTr("Punteggio di iniziativa")
                background: Rectangle {
                    x: 6
                    width: parent.width - 6
                    height: parent.height
                    color: "white"
                    border.color: "#777777"
                }
            }
            InfoButton {
                id: info
                objectName: "initiativeInfo"
                height: parent.height
                Accessible.name: qsTr("Dettagli dell'iniziativa")
            }
        }
        InfoDetails {
            objectName: "initiativeDetails"
            width: content.width - info.width
            visible: info.checked
            rows: root.detailRows
        }
    }
}
