pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum EntryOrigin { Manual, SourceControlled, Derived }
    required property var conditions
    property var automaticConditionIds: []
    signal addConditionRequested(var definition)
    signal removeConditionsRequested(var entryIds)
    spacing: 10

    function manualEntries(condition) {
        return (condition.entries ?? []).filter(entry => entry.origin === ConditionsPanel.Manual);
    }
    function automaticOnly(condition) {
        return automaticConditionIds.includes(condition.id)
            || (condition.effectiveSeverity > 0 && !manualEntries(condition).length);
    }
    function requestAdd(condition, severity, parameter) {
        addConditionRequested({conditionId: condition.id, severity: severity, parameter: parameter});
    }
    function toggle(condition) {
        if (condition.id === "bleeding") {
            bleedingEditor.open();
            return;
        }
        if (automaticOnly(condition)) return;
        const entries = manualEntries(condition);
        if (entries.length) {
            removeConditionsRequested(entries.map(entry => entry.id));
        } else if (condition.entryParameterName || condition.stages?.length > 1) {
            editor.conditionId = condition.id;
            editor.open();
        } else {
            requestAdd(condition, 1, null);
        }
    }

    Dialog {
        id: bleedingEditor
        objectName: "bleedingEditor"
        readonly property var condition: root.conditions?.conditions?.find(entry => entry.id === "bleeding")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, (parent?.width ?? root.width) - 24)
        modal: true
        title: condition?.name ?? qsTr("Sanguinante")
        onAboutToShow: bleedingDamage.clear()
        function submit() {
            const damage = bleedingDamage.text.trim();
            if (!condition || !damage) return;
            root.requestAdd(condition, 1, damage);
            bleedingDamage.clear();
        }
        contentItem: ColumnLayout {
            ListView {
                id: bleedingList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(220, count * 40)
                visible: count > 0
                clip: true
                model: bleedingEditor.condition?.entries ?? []
                ScrollBar.vertical: ScrollBar {}
                delegate: RowLayout {
                    id: bleedingRow
                    required property var modelData
                    width: bleedingList.width
                    height: 40
                    Label {
                        Layout.fillWidth: true
                        text: (bleedingRow.modelData.parameter ?? "") + (bleedingRow.modelData.source ? " · " + bleedingRow.modelData.source : "")
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                    }
                    ToolButton {
                        objectName: "removeBleeding_" + bleedingRow.modelData.id
                        visible: bleedingRow.modelData.origin === ConditionsPanel.Manual
                        text: "×"
                        Accessible.name: qsTr("Rimuovi sanguinamento")
                        onClicked: root.removeConditionsRequested([bleedingRow.modelData.id])
                    }
                }
            }
            Label { visible: !bleedingList.count; text: qsTr("Nessun sanguinamento attivo.") }
            RowLayout {
                TextField {
                    id: bleedingDamage
                    objectName: "bleedingDamage"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Danno per turno, es. 2d6 PF")
                    Accessible.name: placeholderText
                    ContextMenu.menu: null
                    onAccepted: bleedingEditor.submit()
                }
                ToolButton {
                    objectName: "addBleeding"
                    text: "+"
                    Accessible.name: qsTr("Aggiungi sanguinamento")
                    enabled: !!bleedingEditor.condition && !!bleedingDamage.text.trim()
                    onClicked: bleedingEditor.submit()
                }
            }
        }
        standardButtons: Dialog.Close
    }

    Dialog {
        id: editor
        objectName: "conditionEditor"
        property string conditionId: ""
        readonly property var condition: root.conditions?.conditions?.find(entry => entry.id === conditionId)
        readonly property bool valid: !!condition && !root.automaticOnly(condition) && !root.manualEntries(condition).length
            && (!condition.entryParameterName || !!parameter.text.trim())
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: condition?.name ?? qsTr("Applica condizione")
        onAboutToShow: { stage.currentIndex = 0; parameter.clear(); }
        contentItem: ColumnLayout {
            ComboBox {
                id: stage
                visible: count > 1
                Layout.fillWidth: true
                model: editor.condition?.stages ?? []
                textRole: "name"
                valueRole: "severity"
                Accessible.name: qsTr("Grado della condizione")
            }
            TextField {
                id: parameter
                visible: !!editor.condition?.entryParameterName
                Layout.fillWidth: true
                placeholderText: editor.condition?.entryParameterName ?? ""
                Accessible.name: placeholderText
                ContextMenu.menu: null
            }
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button {
                text: qsTr("Applica")
                enabled: editor.valid
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
        onAccepted: if (valid) root.requestAdd(condition, stage.currentValue ?? 1, parameter.text.trim() || null)
    }

    Label { text: qsTr("Condizioni"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    Label {
        visible: !(root.conditions?.conditions?.length)
        text: qsTr("Nessuna condizione disponibile.")
        color: "#536976"
    }
    GridLayout {
        width: root.width
        columns: Math.max(1, Math.floor((width + 8) / 175))
        columnSpacing: 8
        rowSpacing: 8
        Repeater {
            model: (root.conditions?.conditions ?? []).slice().sort((a, b) => a.name.localeCompare(b.name, "it"))
            Button {
                id: tile
                required property var modelData
                readonly property bool applied: modelData.effectiveSeverity > 0
                readonly property bool automatic: root.automaticOnly(modelData)
                readonly property bool stepped: modelData.id === "negativeLevels" || modelData.id === "fear"
                readonly property var manual: root.manualEntries(modelData)
                readonly property string stageName: modelData.stages?.find(stage => stage.severity === modelData.effectiveSeverity)?.name ?? ""
                objectName: "conditionTile_" + modelData.id
                Layout.fillWidth: true
                Layout.preferredWidth: 170
                Layout.preferredHeight: 58
                enabled: stepped || modelData.id === "bleeding" || !automatic
                focusPolicy: stepped ? Qt.NoFocus : Qt.StrongFocus
                text: modelData.name
                Accessible.name: text + (automatic ? qsTr(" · Automatica") : applied ? qsTr(" · Attiva") : "")
                onClicked: if (!stepped) root.toggle(modelData)
                background: Rectangle {
                    radius: 5
                    border.color: tile.visualFocus || tile.hovered ? "#52768e" : tile.applied ? "#87ad96" : "#d2dbe1"
                    color: tile.applied ? "#e1f0e7" : "#fafbfc"
                }
                contentItem: Column {
                    spacing: 2
                    Label {
                        width: parent.width
                        text: tile.text
                        textFormat: Text.PlainText
                        horizontalAlignment: Text.AlignHCenter
                        font.bold: true
                        color: "#253947"
                        elide: Text.ElideRight
                    }
                    Label {
                        width: parent.width
                        text: [tile.applied ? (tile.stageName && tile.stageName !== tile.text ? tile.stageName : qsTr("Attiva")) : "",
                            tile.automatic ? qsTr("Automatica") : ""].filter(value => !!value).join(" · ")
                        textFormat: Text.PlainText
                        horizontalAlignment: Text.AlignHCenter
                        color: "#536976"
                        visible: !tile.stepped && !!text
                        elide: Text.ElideRight
                    }
                    RowLayout {
                        width: parent.width
                        visible: tile.stepped
                        spacing: 2
                        ToolButton {
                            objectName: "decreaseCondition_" + tile.modelData.id
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            padding: 0
                            text: "−"
                            Accessible.name: qsTr("Diminuisci: %1").arg(tile.text)
                            enabled: tile.manual.length > 0
                            onClicked: root.removeConditionsRequested([tile.manual[tile.manual.length - 1].id])
                        }
                        Label {
                            objectName: "conditionStep_" + tile.modelData.id
                            Layout.fillWidth: true
                            text: tile.modelData.id === "fear" ? (tile.stageName || qsTr("Nessuna"))
                                : String(tile.modelData.effectiveSeverity)
                            textFormat: Text.PlainText
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            color: "#536976"
                        }
                        ToolButton {
                            objectName: "increaseCondition_" + tile.modelData.id
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            padding: 0
                            text: "+"
                            Accessible.name: qsTr("Aumenta: %1").arg(tile.text)
                            enabled: tile.modelData.id !== "fear"
                                || tile.modelData.effectiveSeverity < (tile.modelData.stages?.length ?? 0)
                            onClicked: root.requestAdd(tile.modelData, 1, null)
                        }
                    }
                }
            }
        }
    }
}
