pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root
    enum ManeuverType { BullRush, DirtyTrick, Disarm, Drag, Grapple, Overrun, Reposition, Steal, Sunder, Trip }
    required property var combatManeuvers
    property bool prominentHeading: false
    implicitWidth: 410
    implicitHeight: table.height
    function nameFor(type) {
        return [qsTr("Spingere"), qsTr("Sporco trucco"), qsTr("Disarmare"), qsTr("Trascinare"), qsTr("Lottare"),
            qsTr("Oltrepassare"), qsTr("Riposizionare"), qsTr("Rubare"), qsTr("Spezzare"), qsTr("Sbilanciare")][type] ?? "";
    }
    function entryFor(type) { return (combatManeuvers?.maneuvers ?? []).find(entry => entry.type === type); }
    function signed(value) { return value == null ? "—" : (value >= 0 ? "+" : "") + value; }
    function bonusText(entry) {
        return [...new Set((entry?.bonus?.abilityOptions ?? []).map(option => signed(option.totalValue)))].join(" / ") || "—";
    }
    function situationalBonuses(entry) {
        const groups = new Map();
        const totals = [entry?.bonus?.modifiers?.conditionalTotals ?? [], entry?.defense?.modifiers?.conditionalTotals ?? []];
        for (let index = 0; index < totals.length; ++index) {
            for (const {condition, value} of totals[index]) {
                if (!groups.has(condition)) groups.set(condition, [0, 0]);
                groups.get(condition)[index] = value;
            }
        }
        return Array.from(groups, ([condition, values]) => ({condition: condition, values: values, labels: [qsTr("BMC"), qsTr("DMC")]}));
    }
    function detailRows(entry) {
        if (!entry) return [{kind: InfoDetails.Note, text: qsTr("Dati non disponibili.")}];
        const rows = [{kind: InfoDetails.Heading, text: qsTr("BMC")}];
        const abilities = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        const options = entry.bonus?.abilityOptions ?? [];
        for (const option of options) {
            if (options.length > 1) rows.push({kind: InfoDetails.Group, text: option.source});
            rows.push({kind: InfoDetails.Entry, text: signed(combatManeuvers?.baseAttackBonus) + " BAB"},
                {kind: InfoDetails.Entry, text: signed(option.abilityModifier) + " " + (abilities[option.abilityType] ?? qsTr("Caratteristica"))});
            rows.push(...ModifierText.rows(entry.bonus.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            rows.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + signed(option.totalValue)});
        }
        const defense = entry.defense;
        if (defense) {
            rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: qsTr("DMC")},
                {kind: InfoDetails.Entry, text: qsTr("10 Base")},
                {kind: InfoDetails.Entry, text: signed(combatManeuvers?.baseAttackBonus) + " BAB"},
                {kind: InfoDetails.Entry, text: signed(defense.strengthModifier) + qsTr(" Forza")},
                {kind: InfoDetails.Entry, text: signed(defense.appliedDexterityModifier) + qsTr(" Destrezza")});
            rows.push(...ModifierText.rows(defense.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            rows.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + defense.totalValue});
            if (defense.dexterityBonusSuppressed) {
                rows.push({kind: InfoDetails.Note, text: qsTr("Bonus di Destrezza non applicato.")});
                for (const suppression of defense.dexteritySuppressions ?? []) rows.push({kind: InfoDetails.Note, text: suppression.source});
            }
        }
        rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return rows;
    }
    Column {
        id: table
        width: root.prominentHeading ? root.width : Math.min(root.width, root.implicitWidth)
        spacing: 4
        Label {
            visible: root.prominentHeading
            width: parent.width
            height: 42
            text: qsTr("Manovre")
            leftPadding: 12
            font.pixelSize: 20
            font.bold: true
            color: "white"
            verticalAlignment: Text.AlignVCenter
            background: Rectangle { color: "#181818" }
        }
        Row {
            height: 30
            Label { width: table.width - 168; height: parent.height; text: qsTr("Manovra"); verticalAlignment: Text.AlignVCenter }
            Label { width: 70; height: parent.height; text: qsTr("BMC"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            Label { width: 70; height: parent.height; text: qsTr("DMC"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
        Repeater {
            model: 10
            Column {
                id: section
                required property int index
                readonly property var entry: root.entryFor(index)
                width: table.width
                spacing: 4
                Row {
                    width: parent.width
                    height: Math.max(48, nameLabel.implicitHeight, bonusLabel.implicitHeight, defenseLabel.implicitHeight)
                    Label {
                        id: nameLabel
                        objectName: "maneuverName_" + section.index
                        width: table.width - 168
                        height: parent.height
                        padding: 12
                        text: root.nameFor(section.index)
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                        background: Rectangle { color: "#181818" }
                    }
                    Repeater {
                        model: 2
                        Label {
                            required property int index
                            objectName: (index === 0 ? "bmc_" : "dmc_") + section.index
                            width: 70
                            height: parent.height
                            padding: 6
                            text: index === 0 ? root.bonusText(section.entry) : String(section.entry?.defense?.totalValue ?? "—")
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                            color: "#181818"
                            background: Rectangle { x: 6; width: parent.width - 6; height: parent.height; color: "white"; border.color: "#777777" }
                        }
                    }
                    // Hidden size probes let alternative totals wrap without clipping.
                    Label { id: bonusLabel; visible: false; width: 70; padding: 6; font.pixelSize: 20; wrapMode: Text.Wrap; text: root.bonusText(section.entry) }
                    Label { id: defenseLabel; visible: false; width: 70; padding: 6; font.pixelSize: 20; wrapMode: Text.Wrap; text: String(section.entry?.defense?.totalValue ?? "—") }
                    InfoButton {
                        id: info
                        objectName: "maneuverInfo_" + section.index
                        height: parent.height
                        Accessible.name: qsTr("Dettaglio di ") + root.nameFor(section.index)
                    }
                }
                SituationalBonusCards {
                    objectName: "maneuverSituational_" + section.index
                    width: section.width - info.width
                    entries: root.situationalBonuses(section.entry)
                }
                InfoDetails {
                    objectName: "maneuverDetails_" + section.index
                    width: section.width - 28
                    visible: info.checked
                    rows: root.detailRows(section.entry)
                }
            }
        }
    }
}
