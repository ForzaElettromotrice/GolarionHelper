pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root

    enum AbilityType { Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma }
    enum ModifierKind { Bonus, Penalty }
    enum BonusType { Generic, Alchemical, Armor, NaturalArmor, Circumstance, Insight, Competence, Deflection, Luck, Inherent, Morale, Enhancement, Profane, Racial, Resistance, Sacred, Dodge, Shield, Size }

    required property var abilities
    property bool readOnly: false
    signal baseValueChangeRequested(int abilityType, int baseValue)
    implicitWidth: 340
    implicitHeight: table.height

    function abilityFor(type) {
        for (const ability of abilities ?? [])
            if (ability.type === type) return ability;
        return null;
    }

    function abilityName(type) {
        switch (type) {
        case AbilityScoresPanel.Strength: return qsTr("Forza");
        case AbilityScoresPanel.Dexterity: return qsTr("Destrezza");
        case AbilityScoresPanel.Constitution: return qsTr("Costituzione");
        case AbilityScoresPanel.Intelligence: return qsTr("Intelligenza");
        case AbilityScoresPanel.Wisdom: return qsTr("Saggezza");
        case AbilityScoresPanel.Charisma: return qsTr("Carisma");
        default: return "";
        }
    }

    function abbreviation(type) {
        switch (type) {
        case AbilityScoresPanel.Strength: return qsTr("FOR");
        case AbilityScoresPanel.Dexterity: return qsTr("DES");
        case AbilityScoresPanel.Constitution: return qsTr("COS");
        case AbilityScoresPanel.Intelligence: return qsTr("INT");
        case AbilityScoresPanel.Wisdom: return qsTr("SAG");
        case AbilityScoresPanel.Charisma: return qsTr("CAR");
        default: return "";
        }
    }

    function modifierRows(ability, name) {
        const lines = [{kind: InfoDetails.Heading, text: qsTr("Modificatori a ") + name}];
        const set = ability?.modifiers;
        if (!set) {
            lines.push({kind: InfoDetails.Note, text: qsTr("Dettagli dei modificatori non disponibili.")});
            return lines;
        }
        const groups = ModifierText.rows(set, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry);
        if (groups.length) lines.push(...groups);
        else lines.push({kind: InfoDetails.Note, text: qsTr("Nessun bonus o penalità attivo.")});
        lines.push({kind: InfoDetails.Spacer, text: ""},
            {kind: InfoDetails.Total, text: qsTr("Totale modificatori: ") + (set.total >= 0 ? "+" : "") + set.total},
            {kind: InfoDetails.Spacer, text: ""},
            {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return lines;
    }

    Column {
        id: table
        width: Math.min(root.width, root.implicitWidth)
        spacing: 4
        Repeater {
            model: 6
            Column {
                id: section
                required property int index
                width: table.width
                spacing: 4
                Row {
                    id: row
                    readonly property int index: section.index
                    readonly property var ability: root.abilityFor(index)
                    objectName: "ability" + index
                    width: table.width
                    height: 48
                    spacing: 6
                    Rectangle {
                        width: (table.width - 46) / 3
                        height: row.height
                        color: "#181818"
                        Column {
                            anchors.centerIn: parent
                            Label {
                                width: parent.width
                                text: root.abbreviation(row.index)
                                color: "white"
                                font.pixelSize: 20
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text: root.abilityName(row.index)
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                    }
                    Rectangle {
                        id: scoreCell
                        objectName: "scoreCell"
                        width: (table.width - 46) / 3
                        height: row.height
                        color: "white"
                        border.color: "#777777"
                        property int originalBase: 0
                        function resetDraft() {
                            originalBase = row.ability?.baseValue ?? 0;
                            baseInput.text = originalBase > 0 ? String(originalBase) : "";
                        }
                        function submit() {
                            const value = Number(baseInput.text);
                            if (/^\d+$/.test(baseInput.text) && Number.isInteger(value)
                                    && value > 0 && value <= 2147483647
                                    && row.ability?.baseValue === originalBase && value !== originalBase) {
                                root.baseValueChangeRequested(row.index, value);
                            }
                            resetDraft();
                        }
                        HoverHandler {
                            id: scoreHover
                            onHoveredChanged: {
                                if (hovered && !baseInput.activeFocus) scoreCell.resetDraft();
                            }
                        }
                        Label {
                            objectName: "totalValue"
                            visible: root.readOnly || (!scoreHover.hovered && !baseInput.activeFocus)
                            anchors.fill: parent
                            text: row.ability ? String(row.ability.totalValue) : "—"
                            color: "#181818"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            Accessible.name: root.abilityName(row.index) + qsTr(" — punteggio")
                        }
                        TextField {
                            ContextMenu.menu: null
                            id: baseInput
                            objectName: "baseInput"
                            anchors.fill: parent
                            visible: !root.readOnly && (scoreHover.hovered || activeFocus)
                            enabled: row.ability !== null && row.ability.baseValue > 0
                            selectByMouse: true
                            color: "#181818"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            inputMethodHints: Qt.ImhDigitsOnly
                            background: Rectangle { color: "#fff9e9"; border.color: "#777777" }
                            Accessible.name: root.abilityName(row.index) + qsTr(" — punteggio base")
                            onAccepted: { scoreCell.submit(); focus = false; }
                            onEditingFinished: scoreCell.submit()
                            Keys.onEscapePressed: { scoreCell.resetDraft(); focus = false; }
                        }
                    }
                    Rectangle {
                        width: (table.width - 46) / 3
                        height: row.height
                        color: "white"
                        border.color: "#777777"
                        Label {
                            objectName: "modifier"
                            anchors.fill: parent
                            text: row.ability ? String(row.ability.modifier) : "—"
                            color: "#181818"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            Accessible.name: root.abilityName(row.index) + qsTr(" — modificatore")
                        }
                    }
                    InfoButton {
                        id: detailsButton
                        objectName: "detailsButton"
                        height: row.height
                        Accessible.name: root.abilityName(row.index) + qsTr(" — mostra modificatori")
                    }
                }
                InfoDetails {
                    objectName: "modifierDetails"
                    width: table.width - detailsButton.width - row.spacing
                    visible: detailsButton.checked
                    rows: root.modifierRows(row.ability, root.abilityName(row.index))
                }
            }
        }
    }
}
