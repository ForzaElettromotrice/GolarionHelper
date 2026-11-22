pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "ModifierText.js" as ModifierText

Item {
    id: root
    required property var movement
    property bool showHeading: true
    enum MovementType { Land, Climb, Swim, Burrow, Fly }
    enum Maneuverability { Clumsy, Poor, Average, Good, Perfect }
    implicitWidth: 420
    implicitHeight: content.height
    function nameFor(type) {
        return [qsTr("Terra"), qsTr("Scalare"), qsTr("Nuotare"), qsTr("Scavare"), qsTr("Volare")][type] ?? "";
    }
    function distance(units) { return units === null || units === undefined ? "—" : String(units * 1.5).replace(".", ",") + " m"; }
    function maneuverabilityName(value) {
        return [qsTr("Maldestra"), qsTr("Scarsa"), qsTr("Media"), qsTr("Buona"), qsTr("Perfetta")][value] ?? "";
    }
    readonly property var grants: Array.from(movement?.grants ?? []).sort((a, b) => a.type - b.type)
    function detailRows(grant) {
        const rows = [{kind: InfoDetails.Heading, text: nameFor(grant.type)},
            {kind: InfoDetails.Entry, text: grant.source},
            {kind: InfoDetails.Entry, text: qsTr("Velocità base: ") + distance(grant.baseUnits)}];
        if (grant.modifiers) {
            const modifiers = {modifiers: (grant.modifiers.modifiers ?? []).map(entry => Object.assign({}, entry, {resolvedValue: distance(entry.resolvedValue)}))};
            rows.push(...ModifierText.rows(modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
        }
        for (const adjustment of grant.adjustments ?? [])
            rows.push({kind: InfoDetails.Note, text: adjustment.source + " — " + adjustment.description + (adjustment.condition ? " · " + adjustment.condition : "")});
        for (const bonus of grant.boundedBonuses ?? [])
            rows.push({kind: InfoDetails.Note, text: bonus.source + qsTr(" — bonus limitato a ") + distance(bonus.cappedUnits) + qsTr(" (richiesti ") + distance(bonus.requestedUnits) + ")"});
        rows.push({kind: InfoDetails.Total, text: qsTr("Velocità: ") + distance(grant.effectiveUnits)});
        if (!grant.usable) rows.push({kind: InfoDetails.Note, text: qsTr("Movimento non utilizzabile.")});
        if (grant.maneuverability !== null && grant.maneuverability !== undefined)
            rows.push({kind: InfoDetails.Entry, text: qsTr("Manovrabilità: ") + maneuverabilityName(grant.maneuverability)});
        if (grant.run?.supported) {
            rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: qsTr("Corsa")});
            rows.push({kind: InfoDetails.Entry, text: grant.run.usable ? distance(grant.run.distanceUnits) + " (×" + grant.run.effectiveMultiplier + ")" : qsTr("Non utilizzabile")});
            for (const reason of grant.run.notUsableReasons ?? []) rows.push({kind: InfoDetails.Note, text: reason});
            for (const adjustment of grant.run.adjustments ?? [])
                if (adjustment.applied) rows.push({kind: InfoDetails.Note, text: adjustment.source + " — " + adjustment.description});
        }
        return rows;
    }
    Column {
        id: content
        width: Math.min(root.width, root.implicitWidth)
        spacing: 6
        Label { visible: root.showHeading; text: qsTr("Movimento"); font.bold: true; font.pixelSize: 16; color: "#181818" }
        Label { visible: !root.grants.length; text: qsTr("Nessuna modalità di movimento disponibile."); width: parent.width; wrapMode: Text.Wrap; color: "#666666" }
        Repeater {
            model: root.grants
            Column {
                id: section
                required property var modelData
                readonly property bool supportsRunning: modelData.run?.supported === true
                readonly property real distanceWidth: Math.min(80, Math.floor(width * 0.24))
                objectName: "movement_" + modelData.id
                width: content.width
                spacing: 4
                Row {
                    width: parent.width
                    height: Math.max(54, names.implicitHeight + 12, speed.implicitHeight + 12, section.supportsRunning ? runLabels.implicitHeight + 12 : 0)
                    spacing: 6
                    Rectangle {
                        width: Math.max(0, section.width - 46 - 2 * section.distanceWidth)
                        height: parent.height
                        color: "#181818"
                        Column {
                            id: names
                            x: 10
                            width: parent.width - 20
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            Label { width: parent.width; wrapMode: Text.Wrap; text: root.nameFor(section.modelData.type); color: "white"; font.bold: true; font.pixelSize: 16 }
                            Label { text: section.modelData.source; textFormat: Text.PlainText; width: parent.width; wrapMode: Text.Wrap; color: "#dddddd"; font.pixelSize: 11 }
                        }
                    }
                    Label {
                        id: speed
                        objectName: "movementSpeed"
                        width: section.distanceWidth
                        height: parent.height
                        text: section.modelData.usable ? root.distance(section.modelData.effectiveUnits) : "—"
                        font.pixelSize: 20
                        color: "#181818"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "white"; border.color: "#777777" }
                    }
                    Rectangle {
                        width: section.distanceWidth
                        height: parent.height
                        color: "#f2f5f7"
                        border.color: "#c6cfd5"
                        Column {
                            id: runLabels
                            anchors.centerIn: parent
                            width: parent.width - 8
                            spacing: 2
                            Label {
                                width: parent.width
                                text: qsTr("Corsa")
                                color: "#555555"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                objectName: "runDistance"
                                width: parent.width
                                text: section.supportsRunning && section.modelData.run?.usable ? root.distance(section.modelData.run.distanceUnits) : "—"
                                color: "#181818"
                                font.pixelSize: 18
                                wrapMode: Text.Wrap
                                horizontalAlignment: Text.AlignHCenter
                                Accessible.name: qsTr("Distanza di corsa")
                            }
                        }
                    }
                    InfoButton { id: info; objectName: "movementInfo"; height: parent.height; Accessible.name: qsTr("Dettagli movimento: ") + root.nameFor(section.modelData.type) + " — " + section.modelData.source }
                }
                InfoDetails {
                    objectName: "movementDetails"
                    width: parent.width - 34
                    visible: info.checked
                    rows: root.detailRows(section.modelData)
                }
            }
        }
    }
}
