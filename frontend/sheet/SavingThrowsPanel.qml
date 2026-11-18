pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root
    enum SavingThrowType { Fortitude, Reflex, Will }
    required property var savingThrows
    implicitWidth: 260
    implicitHeight: table.height

    function nameFor(type) {
        switch (type) {
        case SavingThrowsPanel.Fortitude: return qsTr("Tempra");
        case SavingThrowsPanel.Reflex: return qsTr("Riflessi");
        case SavingThrowsPanel.Will: return qsTr("Volontà");
        default: return "";
        }
    }
    function savingThrowFor(type) {
        return Array.from(savingThrows ?? []).find(entry => entry.type === type) ?? null;
    }
    function signed(value) { return value === undefined ? "—" : (value >= 0 ? "+" : "") + value; }
    function detailRows(entry) {
        if (!entry) return [{kind: InfoDetails.Note, text: qsTr("Dati non disponibili.")}];
        const lines = [{kind: InfoDetails.Heading, text: qsTr("Punteggio")}];
        const abilities = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        const options = entry.abilityOptions ?? [];
        for (const option of options) {
            if (options.length > 1)
                lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Group, text: option.source});
            const contributions = entry.baseContributions?.contributions ?? [];
            for (const contribution of contributions)
                lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(contribution.resolvedValue) + " " + contribution.id});
            if (!contributions.length)
                lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(entry.baseValue) + qsTr(" Base")});
            lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(option.abilityModifier) + " " + (abilities[option.abilityType] ?? qsTr("Caratteristica non specificata"))});
            lines.push(...ModifierText.rows(entry.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            lines.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + signed(option.totalValue)});
        }
        lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return lines;
    }

    Column {
        id: table
        width: Math.min(root.width, root.implicitWidth)
        spacing: 4
        Row {
            height: 30
            Label { width: table.width - 108; height: parent.height; text: qsTr("Tiro salvezza"); verticalAlignment: Text.AlignVCenter }
            Label { width: 80; height: parent.height; text: qsTr("Punteggio"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
        Repeater {
            model: 3
            Column {
                id: section
                required property int index
                readonly property var entry: root.savingThrowFor(index)
                objectName: "savingThrow" + index
                width: table.width
                spacing: 4
                Rectangle {
                    width: table.width
                    height: Math.max(48, score.implicitHeight + 16)
                    color: "transparent"
                    Row {
                        anchors.fill: parent
                        Label {
                            width: table.width - 108
                            height: parent.height
                            leftPadding: 14
                            rightPadding: 12
                            text: root.nameFor(section.index)
                            verticalAlignment: Text.AlignVCenter
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                            background: Rectangle { color: "#181818" }
                        }
                        Label {
                            id: score
                            objectName: "score"
                            width: 80
                            height: parent.height
                            leftPadding: 6
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                const values = Array.from(section.entry?.abilityOptions ?? []).map(option => option.totalValue);
                                return values.length ? [...new Set(values)].map(value => root.signed(value)).join(" / ") : "—";
                            }
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                            color: "#181818"
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
                            objectName: "savingThrowInfo"
                            height: parent.height
                            Accessible.name: qsTr("Dettaglio di ") + root.nameFor(section.index)
                        }
                    }
                }
                InfoDetails {
                    objectName: "savingThrowDetails"
                    width: section.width - info.width
                    visible: info.checked
                    rows: root.detailRows(section.entry)
                }
            }
        }
    }
}
