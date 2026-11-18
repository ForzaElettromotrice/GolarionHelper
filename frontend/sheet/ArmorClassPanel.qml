pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root
    enum ArmorClassType { Normal, Touch, FlatFooted }
    required property var armorClass
    implicitWidth: 340
    implicitHeight: table.height

    function nameFor(type) {
        switch (type) {
        case ArmorClassPanel.Normal: return qsTr("CA totale");
        case ArmorClassPanel.Touch: return qsTr("Contatto");
        case ArmorClassPanel.FlatFooted: return qsTr("Impreparato");
        default: return "";
        }
    }
    function optionsFor(type) {
        return Array.from(armorClass?.abilityOptions ?? []).map(option => ({
            option: option, value: (option.values ?? []).find(value => value.type === type)
        })).filter(entry => entry.value !== undefined);
    }
    function signed(value) { return value === undefined ? "—" : (value >= 0 ? "+" : "") + value; }
    function detailRows(type) {
        const options = optionsFor(type);
        if (!options.length) return [{kind: InfoDetails.Note, text: qsTr("Dati non disponibili.")}];
        const lines = [{kind: InfoDetails.Heading, text: nameFor(type)}];
        const abilities = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        for (const entry of options) {
            if (options.length > 1)
                lines.push({kind: InfoDetails.Group, text: entry.option.source});
            lines.push({kind: InfoDetails.Entry, indent: 2, text: qsTr("10 Base")});
            const limit = armorClass.maximumDexterityBonus;
            const limitText = entry.option.abilityType === AbilityScoresPanel.Dexterity
                && type !== ArmorClassPanel.FlatFooted && limit !== null && limit !== undefined
                ? qsTr(" (Max ") + limit + ")" : "";
            lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(entry.value.appliedAbilityModifier) + " " + abilities[entry.option.abilityType] + limitText});
            lines.push(...ModifierText.rows(entry.value.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            lines.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + entry.value.totalValue});
        }
        for (const suppression of armorClass.abilitySuppressions ?? [])
            lines.push({kind: InfoDetails.Note, text: suppression.source + (suppression.dexterityRetained ? qsTr(" — Destrezza mantenuta") : qsTr(" — soppressione del bonus"))});
        lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return lines;
    }

    Column {
        id: table
        width: Math.min(root.width, root.implicitWidth)
        spacing: 4
        Item {
            width: parent.width
            height: valuesRow.height
        Row {
            id: valuesRow
            width: parent.width
            spacing: 8
            Repeater {
                model: [ArmorClassPanel.Touch, ArmorClassPanel.Normal, ArmorClassPanel.FlatFooted]
                Item {
                    id: cell
                    required property int modelData
                    readonly property bool primary: modelData === ArmorClassPanel.Normal
                    objectName: "armorClass" + modelData
                    width: primary ? table.width * 0.36 : (table.width * 0.64 - 16) / 2
                    height: Math.max(144, labels.implicitHeight + 44)
                    Canvas {
                        width: parent.width
                        height: cell.primary ? parent.height : Math.max(108, labels.implicitHeight + 40)
                        anchors.verticalCenter: parent.verticalCenter
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            function shield(inset) {
                                ctx.beginPath();
                                ctx.moveTo(inset, 10 + inset);
                                ctx.quadraticCurveTo(width / 2, inset - 6, width - inset, 10 + inset);
                                ctx.lineTo(width - inset, height * 0.58);
                                ctx.quadraticCurveTo(width - inset, height * 0.8, width / 2, height - inset);
                                ctx.quadraticCurveTo(inset, height * 0.8, inset, height * 0.58);
                                ctx.closePath();
                            }
                            shield(3);
                            const gradient = ctx.createLinearGradient(0, 0, width, height);
                            gradient.addColorStop(0, "#f3f6f8");
                            gradient.addColorStop(1, "#c8d1d9");
                            ctx.fillStyle = gradient;
                            ctx.fill();
                            ctx.strokeStyle = "#596b7b";
                            ctx.lineWidth = 1.5;
                            ctx.stroke();
                            shield(9);
                            ctx.strokeStyle = "#96a5b2";
                            ctx.lineWidth = 1;
                            ctx.stroke();
                        }
                    }
                    Column {
                        id: labels
                        width: parent.width - 16
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: cell.primary ? -16 : -8
                        spacing: 5
                        Label {
                            width: parent.width
                            text: cell.primary ? qsTr("CA") : root.nameFor(cell.modelData)
                            color: "#526473"
                            font.pixelSize: cell.primary ? 17 : 12
                            font.bold: cell.primary
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Label {
                            objectName: "score"
                            width: parent.width
                            text: {
                                const values = root.optionsFor(cell.modelData).map(entry => entry.value.totalValue);
                                return values.length ? [...new Set(values)].join(" / ") : "—";
                            }
                            color: "#263b4c"
                            font.pixelSize: cell.primary ? 34 : 23
                            font.bold: cell.primary
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                            Accessible.name: root.nameFor(cell.modelData)
                        }
                    }
                }
            }
        }
        InfoButton {
            id: info
            objectName: "armorClassInfo"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            height: 24
            Accessible.name: qsTr("Dettagli della Classe Armatura")
        }
        }
        InfoDetails {
            objectName: "armorClassDetails"
            width: parent.width
            visible: info.checked
            rows: {
                const result = [];
                for (const type of [ArmorClassPanel.Normal, ArmorClassPanel.Touch, ArmorClassPanel.FlatFooted]) {
                    if (result.length) result.push({kind: InfoDetails.Spacer, text: ""});
                    result.push(...root.detailRows(type));
                }
                return result;
            }
        }
    }
}
