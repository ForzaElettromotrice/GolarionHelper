pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    required property var actions
    property var attackRoutines: null
    property bool prominentHeading: false
    property var collapsedCategoryIds: []
    readonly property var visibleCategories: {
        const routineActions = new Set((attackRoutines?.routines ?? []).map(routine => routine.actionId));
        return (actions?.categories ?? []).map(category => ({id: category.id ?? category.name, name: category.name,
            actions: (category.actions ?? []).filter(action => !routineActions.has(action.id))}))
            .filter(category => category.actions.length > 0);
    }
    spacing: 12
    function toggleCategory(id) {
        collapsedCategoryIds = collapsedCategoryIds.includes(id)
            ? collapsedCategoryIds.filter(entry => entry !== id) : collapsedCategoryIds.concat(id);
    }
    function detailRows(action) {
        const rows = [];
        if (action.description) rows.push({kind: InfoDetails.Entry, text: action.description});
        if (action.source) rows.push({kind: InfoDetails.Note, text: qsTr("Fonte: %1").arg(action.source)});
        for (const note of action.notes ?? [])
            rows.push({kind: InfoDetails.Entry, text: note.source ? qsTr("%1 · %2").arg(note.text).arg(note.source) : note.text});
        return rows.length ? rows : [{kind: InfoDetails.Note, text: qsTr("Nessun dettaglio disponibile.")}];
    }

    Label {
        width: root.prominentHeading ? parent.width : implicitWidth
        height: root.prominentHeading ? 42 : implicitHeight
        text: qsTr("Azioni")
        leftPadding: root.prominentHeading ? 12 : 0
        font.pixelSize: 20
        font.bold: true
        color: root.prominentHeading ? "white" : "#253947"
        verticalAlignment: Text.AlignVCenter
        background: Rectangle { color: "#181818"; visible: root.prominentHeading }
    }
    Label { visible: !root.visibleCategories.length; text: qsTr("Nessuna azione da mostrare."); color: "#536976" }
    Repeater {
        model: root.visibleCategories
        Column {
            id: category
            required property var modelData
            readonly property bool collapsed: root.collapsedCategoryIds.includes(modelData.id)
            width: root.width
            spacing: 5
            ItemDelegate {
                objectName: "actionCategory_" + category.modelData.id
                width: category.width
                padding: 4
                Accessible.name: (category.collapsed ? qsTr("Apri %1") : qsTr("Chiudi %1")).arg(category.modelData.name)
                onClicked: root.toggleCategory(category.modelData.id)
                background: Item {}
                contentItem: Label {
                    text: (category.collapsed ? "▸ " : "▾ ") + (category.modelData.name ?? qsTr("Azioni"))
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    font.pixelSize: root.prominentHeading ? 17 : 14
                    font.bold: true
                    color: "#253947"
                }
            }
            Repeater {
                model: category.modelData.actions
                Rectangle {
                    id: card
                    required property var modelData
                    objectName: "actionCard_" + modelData.id
                    visible: !category.collapsed
                    width: category.width
                    implicitHeight: actionRow.implicitHeight + 16 + (details.visible ? details.implicitHeight + 8 : 0)
                    color: modelData.usable === false ? "#e9eef1" : "#fafbfc"
                    border.color: "#d2dbe1"
                    radius: 5
                    HoverHandler { id: cardHover }
                    RowLayout {
                        id: actionRow
                        x: 10; y: 8
                        width: parent.width - 20
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Item {
                                Layout.fillWidth: true
                                implicitHeight: Math.max(actionName.implicitHeight, actionInfo.implicitHeight)
                                Label {
                                    id: actionName
                                    objectName: "actionName"
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: Math.min(implicitWidth, Math.max(0, parent.width - actionInfo.width - 4))
                                    text: card.modelData.name
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    font.bold: true
                                    color: "#253947"
                                }
                                InfoButton {
                                    id: actionInfo
                                    objectName: "actionInfo"
                                    anchors.left: actionName.right
                                    anchors.leftMargin: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    opacity: cardHover.hovered || visualFocus ? 1 : 0
                                    Accessible.name: qsTr("Informazioni su %1").arg(card.modelData.name)
                                }
                            }
                            Label {
                                objectName: "actionInhibition"
                                Layout.fillWidth: true
                                visible: card.modelData.usable === false
                                text: {
                                    if (!visible) return "";
                                    const reasons = (card.modelData.inhibitions ?? []).map(entry => entry.reason || entry.source).filter(value => !!value);
                                    return reasons.length ? qsTr("Azione impedita: %1").arg(reasons.join("; ")) : qsTr("Azione impedita");
                                }
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                color: "#536976"
                            }
                        }
                        ActionCostIcon {
                            objectName: "actionCost"
                            cost: card.modelData.effectiveCost ?? -1
                            opacity: card.modelData.usable === false ? 0.55 : 1
                        }
                    }
                    InfoDetails {
                        id: details
                        objectName: "actionDetails"
                        x: actionRow.x
                        y: actionRow.y + actionRow.height + 8
                        width: actionRow.width
                        visible: actionInfo.checked
                        rows: visible ? root.detailRows(card.modelData) : []
                    }
                }
            }
        }
    }
}
