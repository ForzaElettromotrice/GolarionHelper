pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText
import "ShieldShape.js" as ShieldShape

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
    function situationalBonuses() {
        const options = optionsFor(ArmorClassPanel.Normal).map(entry =>
            new Map((entry.value.modifiers?.conditionalTotals ?? []).map(total => [total.condition, total.value])));
        const conditions = new Set();
        for (const option of options)
            for (const condition of option.keys()) conditions.add(condition);
        return Array.from(conditions, condition => ({
            condition: condition,
            values: [...new Set(options.map(option => option.get(condition) ?? 0))]
        }));
    }
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
        readonly property real primaryShieldWidth: width * 0.36
        width: Math.min(root.width, 360)
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4
        Item {
            width: parent.width
            height: valuesRow.height + info.height
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
                    width: primary ? table.primaryShieldWidth : (table.width - table.primaryShieldWidth - 2 * valuesRow.spacing) / 2
                    height: Math.max(124, table.primaryShieldWidth * ShieldShape.heightToWidthRatio, labels.implicitHeight + 44)
                    Canvas {
                        width: parent.width
                        height: cell.primary ? parent.height : Math.max(94, width * ShieldShape.heightToWidthRatio, labels.implicitHeight + 40)
                        anchors.verticalCenter: parent.verticalCenter
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ShieldShape.trace(ctx, width, height);
                            ctx.fillStyle = cell.primary ? "#181818" : "#ffffff";
                            ctx.fill();
                            ctx.strokeStyle = cell.primary ? "#181818" : "#777777";
                            ctx.lineJoin = "round";
                            ctx.lineWidth = cell.primary ? 2.5 : 1.5;
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
                            color: cell.primary ? "#ffffff" : "#555555"
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
                            color: cell.primary ? "#ffffff" : "#181818"
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
            height: 24
            Accessible.name: qsTr("Dettagli della Classe Armatura")
        }
        }
        SituationalBonusCards {
            x: (table.width - width) / 2
            width: table.primaryShieldWidth
            entries: root.situationalBonuses()
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
