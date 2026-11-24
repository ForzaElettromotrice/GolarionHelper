pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var attacks
    property var attackRoutines: ({routines: []})
    property var actions: ({categories: []})
    property var baseAttackBonus: null
    property bool prominentHeading: false
    property var expandedRoutineIds: []
    onAttacksChanged: {
        const present = new Set((attacks?.attacks ?? []).map(attack => attack.id));
        expandedRoutineIds = expandedRoutineIds.filter(id => present.has(id));
    }
    function setRoutineExpanded(id, expanded) {
        const wasExpanded = expandedRoutineIds.includes(id);
        if (expanded === wasExpanded) return;
        expandedRoutineIds = expanded ? [...expandedRoutineIds, id] : expandedRoutineIds.filter(entry => entry !== id);
    }
    enum SelectionMode { ChooseOne, AllMatching }
    function signed(value) { return value >= 0 ? "+" + value : String(value); }
    function attackBonus(attack) {
        return [...new Set((attack.strike?.attackAbilityOptions ?? []).map(option => signed(option.attackBonus)))].join(" / ") || "—";
    }
    function confirmationBonus(attack) {
        return [...new Set((attack.strike?.attackAbilityOptions ?? [])
            .filter(option => option.criticalConfirmationBonus !== undefined && option.criticalConfirmationBonus !== null)
            .map(option => signed(option.criticalConfirmationBonus)))].join(" / ") || "—";
    }
    function hasDifferentConfirmation(attack) {
        return (attack.strike?.attackAbilityOptions ?? []).some(option =>
            option.criticalConfirmationBonus !== undefined && option.criticalConfirmationBonus !== null
            && option.criticalConfirmationBonus !== option.attackBonus);
    }
    function situationalBonuses(set) {
        return (set?.conditionalTotals ?? []).filter(entry => entry.value !== 0);
    }
    component SituationalLines: Column {
        id: lines
        required property var entries
        visible: entries.length > 0
        spacing: 2
        Repeater {
            model: lines.entries
            RowLayout {
                id: line
                required property var modelData
                required property int index
                x: 10
                width: Math.min(implicitWidth, Math.max(0, lines.width - 10))
                spacing: 6
                Item {
                    id: connector
                    Layout.preferredWidth: 10
                    Layout.minimumHeight: 12
                    Layout.fillHeight: true
                    Rectangle {
                        objectName: "attackSituationalSpine"
                        x: 2
                        y: line.index === 0 ? 0 : -lines.spacing
                        width: 1
                        height: (line.index === lines.entries.length - 1 ? Math.floor(connector.height / 2) + 1 : connector.height) - y
                        color: "#888888"
                    }
                    Rectangle { x: 2; y: Math.floor(connector.height / 2); width: 8; height: 1; color: "#888888" }
                }
                Label {
                    Layout.fillWidth: true
                    text: modelData.condition
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                    color: "#555555"
                }
                Label {
                    objectName: "attackSituationalValue"
                    text: (modelData.value > 0 ? "+" : "") + modelData.value
                    font.pixelSize: 12
                    font.bold: true
                    color: "#181818"
                }
            }
        }
    }
    function damageSummary(damage) {
        if (!damage) return "—";
        const dice = (damage.components ?? []).map(entry => {
            const value = entry.component.effectiveDice;
            const expression = value.expression || (value.diceCount + "d" + value.dieSize);
            return entry.occurrences === 1 ? expression : entry.occurrences + " × (" + expression + ")";
        }).join(" + ");
        const bonuses = [...new Set((damage.abilityOptions ?? []).map(option => option.bonus))];
        return (bonuses.length ? bonuses : [0]).map(bonus => dice
            ? dice + (bonus ? " " + signed(bonus) : "") : String(bonus)).join(" / ");
    }
    function criticalSummary(attack) {
        const profile = attack.strike?.criticalProfile;
        if (!profile) return "—";
        return (profile.threatMinimum === 20 ? "20" : profile.threatMinimum + "–20") + " / ×" + profile.multiplier;
    }
    function slotSelection(slot) {
        const ids = slot.effectiveGrantIds ?? [];
        if (!ids.length) {
            if (slot.assignedGrantId) return qsTr("Assegnazione non disponibile");
            return slot.selectionMode === AttacksPanel.AllMatching
                ? qsTr("Nessun attacco compatibile") : qsTr("Nessun attacco assegnato");
        }
        return ids.map(id => (slot.candidates ?? []).find(candidate => candidate.grantId === id)?.name
            ?? qsTr("Attacco non disponibile")).join("\n");
    }
    function actionFor(actionId) {
        for (const category of actions?.categories ?? []) {
            const action = (category.actions ?? []).find(entry => entry.id === actionId);
            if (action) return action;
        }
        return null;
    }
    function actionCost(actionId) { return actionFor(actionId)?.effectiveCost ?? -1; }
    signal addRequested(string routineId)
    signal assignRequested(string attackId, string slotId, string strikeGrantId)
    signal removeRequested(string attackId)
    signal renameRequested(string attackId, string name)
    signal reorderRequested(string attackId, var beforeAttackId)
    function reorderPlacement(movingId, targetId, after) {
        const ids = (attacks?.attacks ?? []).map(attack => attack.id);
        const sourceIndex = ids.indexOf(movingId);
        if (movingId === targetId || sourceIndex < 0 || !ids.includes(targetId)) return null;
        const remaining = ids.filter(id => id !== movingId);
        const before = after ? remaining[remaining.indexOf(targetId) + 1] ?? null : targetId;
        return before === (ids[sourceIndex + 1] ?? null) ? null : {beforeAttackId: before};
    }
    implicitWidth: prominentHeading ? 440 : 360
    implicitHeight: content.height
    Popup {
        id: renamePopup
        objectName: "routineRenamePopup"
        anchors.centerIn: Overlay.overlay
        width: Math.min(340, root.width)
        padding: 12
        focus: true
        property string attackId: ""
        property string originalName: ""
        function edit(id, name) {
            attackId = id;
            originalName = name;
            renameInput.text = name;
            open();
        }
        onOpened: {
            renameInput.forceActiveFocus();
            renameInput.selectAll();
        }
        contentItem: TextField {
            id: renameInput
            objectName: "routineRenameInput"
            ContextMenu.menu: null
            placeholderText: qsTr("Nome della routine")
            Accessible.name: placeholderText
            onAccepted: {
                const name = text.trim();
                if (!name) return;
                renamePopup.close();
                if (name !== renamePopup.originalName) root.renameRequested(renamePopup.attackId, name);
            }
        }
    }
    Column {
        id: content
        width: root.prominentHeading ? root.width : Math.min(root.width, root.implicitWidth)
        spacing: 6
        Row {
            width: parent.width
            Label {
                width: parent.width - 66
                height: root.prominentHeading ? 42 : 32
                text: root.prominentHeading ? qsTr("Routine") : qsTr("Routine di attacco")
                leftPadding: root.prominentHeading ? 12 : 0
                font.pixelSize: root.prominentHeading ? 20 : 16
                font.bold: true
                color: root.prominentHeading ? "white" : "#181818"
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { width: content.width; color: "#181818"; visible: root.prominentHeading }
            }
            ToolButton {
                id: removalLock
                objectName: "routineRemovalLock"
                width: 32
                height: root.prominentHeading ? 42 : 32
                readonly property color ink: root.prominentHeading
                    ? (checked ? "#ffffff" : "#cccccc")
                    : (checked ? "#1769aa" : "#555555")
                padding: 0
                checkable: true
                Accessible.name: checked ? qsTr("Blocca rimozione routine") : qsTr("Sblocca rimozione routine")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                background: Item {}
                contentItem: Item {
                    Rectangle {
                        x: removalLock.checked ? 13 : 10
                        y: (parent.height - 32) / 2 + 5.5
                        width: 10; height: 13
                        radius: 5
                        color: "transparent"
                        border.width: 2
                        border.color: removalLock.ink
                        rotation: removalLock.checked ? 35 : 0
                    }
                    Rectangle {
                        x: 7; y: (parent.height - 32) / 2 + 14.5
                        width: 16; height: 12
                        radius: 2
                        color: removalLock.ink
                        Rectangle { anchors.centerIn: parent; width: 2; height: 5; radius: 1; color: root.prominentHeading ? "#181818" : "white" }
                    }
                }
            }
            ToolButton {
                id: addButton
                readonly property alias routinesPopup: routinesMenu
                objectName: "addRoutine"
                width: 32
                height: root.prominentHeading ? 42 : 32
                palette.buttonText: root.prominentHeading ? "white" : "#181818"
                text: "+"
                font.pixelSize: 24
                Accessible.name: qsTr("Aggiungi routine di attacco")
                onClicked: routinesMenu.visible ? routinesMenu.close() : routinesMenu.open()
                onVisibleChanged: if (!visible) routinesMenu.close()
                Menu {
                    id: routinesMenu
                    width: content.width
                    x: addButton.width - width
                    y: addButton.height
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                    MenuItem {
                        visible: !(root.attackRoutines?.routines?.length)
                        height: visible ? implicitHeight : 0
                        enabled: false
                        text: qsTr("Nessuna routine disponibile")
                    }
                    Instantiator {
                        model: root.attackRoutines?.routines ?? []
                        delegate: MenuItem {
                            id: routineChoice
                            required property var modelData
                            text: modelData.name
                            enabled: modelData.usable !== false
                            contentItem: Column {
                                spacing: 3
                                Label {
                                    width: parent.width
                                    text: routineChoice.text
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    color: routineChoice.enabled ? "#253947" : "#777777"
                                }
                                Label {
                                    visible: !routineChoice.enabled
                                    width: parent.width
                                    text: (routineChoice.modelData.failureReasons ?? []).join("\n") || qsTr("Routine non utilizzabile")
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 12
                                    color: "#805039"
                                }
                            }
                            onTriggered: root.addRequested(modelData.id)
                        }
                        onObjectAdded: (index, object) => routinesMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => routinesMenu.removeItem(object)
                    }
                }
            }
        }
        Label {
            visible: !(root.attacks?.attacks?.length)
            width: parent.width
            text: qsTr("Nessuna routine configurata.")
            color: "#666666"
            wrapMode: Text.Wrap
        }
        Repeater {
            model: root.attacks?.attacks ?? []
            Column {
                required property var modelData
                id: routine
                readonly property var action: root.actionFor(modelData.actionId)
                readonly property bool blocked: action?.usable === false
                readonly property string blockReason: (action?.inhibitions ?? []).map(entry => entry.reason).filter(reason => !!reason).join("\n") || qsTr("L’azione non è utilizzabile.")
                width: content.width
                spacing: 4
                Row {
                    id: routineHeader
                    width: parent.width
                    spacing: removalLock.checked ? 4 : 0
                    DragHandler {
                        id: routineDrag
                        target: null
                        enabled: (root.attacks?.attacks?.length ?? 0) > 1
                        onActiveChanged: {
                            if (active) routineDragMarker.Drag.active = true;
                            else if (routineDragMarker.Drag.active) routineDragMarker.Drag.drop();
                        }
                        onCanceled: routineDragMarker.Drag.cancel()
                    }
                    Item {
                        id: routineDragMarker
                        readonly property string routineId: routine.modelData.id
                        x: routineDrag.centroid.position.x
                        y: routineDrag.centroid.position.y
                        Drag.keys: ["attack-routine"]
                        Drag.supportedActions: Qt.MoveAction
                        Drag.proposedAction: Qt.MoveAction
                    }
                    Rectangle {
                        parent: Overlay.overlay
                        visible: routineDrag.active
                        z: 1
                        readonly property point pointer: visible && parent
                            ? routineHeader.mapToItem(parent, routineDrag.centroid.position.x, routineDrag.centroid.position.y)
                            : Qt.point(0, 0)
                        x: pointer.x + 12
                        y: pointer.y + 10
                        width: Math.min(240, dragName.implicitWidth + 18)
                        height: dragName.implicitHeight + 12
                        radius: 5
                        color: "#fafbfc"
                        border.color: "#52768e"
                        Label {
                            id: dragName
                            anchors.fill: parent
                            anchors.margins: 6
                            text: routine.modelData.name
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                        }
                    }
                Item {
                    width: parent.width - (removalLock.checked ? 32 : 0)
                    height: header.height
                    HoverHandler { id: routineTitleHover }
                Button {
                    id: header
                    objectName: "routine_" + routine.modelData.id
                    width: parent.width
                    padding: 12
                    rightPadding: routine.modelData.complete === false ? 108 : 80
                    topPadding: 16
                    bottomPadding: 16
                    text: routine.modelData.name
                    checkable: true
                    checked: !routine.blocked && root.expandedRoutineIds.includes(routine.modelData.id)
                    enabled: !routine.blocked
                    onToggled: root.setRoutineExpanded(routine.modelData.id, checked)
                    onEnabledChanged: if (!enabled) root.setRoutineExpanded(routine.modelData.id, false)
                    Accessible.description: routine.blocked ? routine.blockReason : (routine.modelData.complete === false ? qsTr("Routine incompleta") : "")
                    contentItem: Label {
                        id: routineTitle
                        text: header.text
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        color: "#181818"
                    }
                    background: Rectangle {
                        color: routine.blocked ? "#e8e8e8" : (header.checked || header.hovered ? "#e6edf2" : "#f2f5f7")
                        border.color: header.activeFocus ? "#52768e" : "#c6cfd5"
                        radius: 5
                        Item {
                            anchors.fill: parent
                            visible: routine.blocked
                            clip: true
                            Repeater {
                                model: Math.ceil(header.width / 12) + 12
                                Rectangle {
                                    required property int index
                                    x: index * 12 - 70
                                    y: -header.height
                                    width: 2
                                    height: header.height * 3
                                    rotation: 35
                                    color: "#38909aa2"
                                }
                            }
                        }
                    }
                    Label {
                        objectName: "incomplete_" + routine.modelData.id
                        visible: routine.modelData.complete === false
                        anchors.right: parent.right
                        anchors.rightMargin: 50
                        anchors.verticalCenter: parent.verticalCenter
                        text: "!"
                        font.pixelSize: 26
                        font.bold: true
                        color: "#b76b12"
                        Accessible.name: qsTr("Routine incompleta")
                        ToolTip.visible: warningHover.hovered
                        ToolTip.text: qsTr("Routine incompleta")
                        HoverHandler { id: warningHover }
                    }
                    ActionCostIcon {
                        objectName: "cost_" + routine.modelData.id
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        cost: root.actionCost(routine.modelData.actionId)
                    }
                }
                ToolButton {
                    id: renameButton
                    objectName: "rename_" + routine.modelData.id
                    visible: routineTitleHover.hovered
                    x: routineTitle.x + Math.ceil(Math.min(routineTitle.contentWidth, routineTitle.width)) + 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    padding: 0
                    Accessible.name: qsTr("Rinomina routine %1").arg(routine.modelData.name)
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    background: Item {}
                    contentItem: Item {
                        Canvas {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            readonly property color ink: renameButton.hovered || renameButton.visualFocus ? "#181818" : "#555555"
                            onInkChanged: requestPaint()
                            onPaint: {
                                const ctx = getContext("2d");
                                ctx.clearRect(0, 0, width, height);
                                ctx.strokeStyle = ink;
                                ctx.lineWidth = 1.4;
                                ctx.lineJoin = "round";
                                ctx.beginPath();
                                ctx.moveTo(3, 10);
                                ctx.lineTo(11, 2);
                                ctx.lineTo(14, 5);
                                ctx.lineTo(6, 13);
                                ctx.lineTo(2, 14);
                                ctx.closePath();
                                ctx.moveTo(9, 4);
                                ctx.lineTo(12, 7);
                                ctx.stroke();
                            }
                        }
                    }
                    onClicked: renamePopup.edit(routine.modelData.id, routine.modelData.name)
                }
                }
                ToolButton {
                    id: removeButton
                    objectName: "remove_" + routine.modelData.id
                    visible: removalLock.checked
                    width: 28
                    height: header.height
                    text: "×"
                    Accessible.name: qsTr("Rimuovi routine ") + routine.modelData.name
                    background: Item {}
                    contentItem: Label {
                        text: "×"
                        font.pixelSize: 22
                        color: removeButton.hovered ? "#b33434" : "#777777"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.removeRequested(routine.modelData.id)
                }
                }
                DropArea {
                    id: routineDrop
                    parent: root
                    x: content.x + routine.x
                    y: content.y + routine.y - 2
                    width: routine.width
                    height: routine.height + 4
                    keys: ["attack-routine"]
                    readonly property bool insertAfter: drag.y > height / 2
                    readonly property var placement: containsDrag
                        ? root.reorderPlacement(drag.source?.routineId, routine.modelData.id, insertAfter) : null
                    onDropped: drop => {
                        const movingId = drop.source?.routineId;
                        const target = root.reorderPlacement(movingId, routine.modelData.id, drop.y > height / 2);
                        if (!target) return;
                        drop.acceptProposedAction();
                        root.reorderRequested(movingId, target.beforeAttackId);
                    }
                    Rectangle {
                        objectName: "routineInsertion_" + routine.modelData.id
                        width: parent.width
                        height: 2
                        y: routineDrop.insertAfter ? parent.height - 1 : -1
                        visible: !!routineDrop.placement
                        color: "#1769aa"
                    }
                }
                Label {
                    objectName: "blockedReason_" + routine.modelData.id
                    visible: routine.blocked
                    width: parent.width
                    leftPadding: 12
                    rightPadding: 12
                    bottomPadding: 6
                    text: routine.blockReason
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: "#805039"
                }
                Rectangle {
                    objectName: "slots_" + routine.modelData.id
                    visible: header.checked && !routine.blocked
                    width: parent.width
                    implicitHeight: slotsContent.height + 24
                    color: "#f8fafb"
                    border.color: "#c6cfd5"
                    radius: 5
                    Column {
                        id: slotsContent
                        x: 12; y: 12
                        width: parent.width - 24
                        spacing: 12
                        Row {
                            width: parent.width
                            Label {
                                width: parent.width - 28
                                height: 28
                                text: qsTr("Slot")
                                font.bold: true
                                color: "#253947"
                                verticalAlignment: Text.AlignVCenter
                            }
                            InfoButton {
                                id: routineInfo
                                objectName: "routineInfo_" + routine.modelData.id
                                height: 28
                                Accessible.name: qsTr("Mostra informazioni della routine")
                            }
                        }
                        InfoDetails {
                            visible: routineInfo.checked && rows.length > 0
                            width: parent.width
                            framed: false
                            padding: 0
                            rows: (routine.modelData.failureReasons ?? []).map(reason => ({kind: InfoDetails.Note, text: reason}))
                        }
                        Label {
                            visible: !(routine.modelData.slots?.length)
                            width: parent.width
                            text: qsTr("Nessuno slot disponibile.")
                            color: "#666666"
                            wrapMode: Text.Wrap
                        }
                        Repeater {
                            model: routine.modelData.slots ?? []
                            Column {
                                id: slotRow
                                required property var modelData
                                readonly property bool manual: modelData.selectionMode === AttacksPanel.ChooseOne
                                readonly property var choices: (modelData.candidates ?? []).filter(candidate => candidate.accepted === true)
                                width: slotsContent.width
                                spacing: 3
                                Label {
                                    width: parent.width
                                    text: slotRow.modelData.name
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#253947"
                                }
                                Button {
                                    id: selectionButton
                                    readonly property alias choicesPopup: choicesMenu
                                    objectName: "choose_" + routine.modelData.id + "_" + slotRow.modelData.id
                                    width: parent.width
                                    enabled: slotRow.manual && !routine.blocked
                                    padding: slotRow.manual ? 10 : 0
                                    rightPadding: slotRow.manual ? 30 : 0
                                    onClicked: choicesMenu.visible ? choicesMenu.close() : choicesMenu.open()
                                    onVisibleChanged: if (!visible) choicesMenu.close()
                                    onEnabledChanged: if (!enabled) choicesMenu.close()
                                    Accessible.name: qsTr("Scegli attacco per ") + slotRow.modelData.name
                                    contentItem: Label {
                                        objectName: "selection_" + routine.modelData.id + "_" + slotRow.modelData.id
                                        text: root.slotSelection(slotRow.modelData)
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 14
                                        color: "#555555"
                                    }
                                    background: Rectangle {
                                        visible: slotRow.manual
                                        color: selectionButton.hovered ? "#edf3f7" : "white"
                                        border.color: selectionButton.activeFocus ? "#52768e" : "#c6cfd5"
                                        radius: 4
                                    }
                                    Label {
                                        visible: slotRow.manual
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "▾"
                                        color: "#52768e"
                                    }
                                    Menu {
                                        id: choicesMenu
                                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                                        objectName: "choices_" + routine.modelData.id + "_" + slotRow.modelData.id
                                        y: selectionButton.height
                                        width: selectionButton.width
                                        MenuItem {
                                            visible: slotRow.choices.length === 0
                                            height: visible ? implicitHeight : 0
                                            enabled: false
                                            text: qsTr("Nessun attacco selezionabile")
                                        }
                                        Instantiator {
                                            model: slotRow.choices
                                            delegate: MenuItem {
                                                id: choice
                                                required property var modelData
                                                objectName: "candidate_" + routine.modelData.id + "_" + slotRow.modelData.id + "_" + modelData.grantId
                                                text: modelData.name
                                                checkable: true
                                                checked: slotRow.modelData.assignedGrantId === modelData.grantId
                                                contentItem: Label {
                                                    text: choice.text
                                                    textFormat: Text.PlainText
                                                    wrapMode: Text.Wrap
                                                    leftPadding: 24
                                                    color: "#253947"
                                                }
                                                onTriggered: {
                                                    checked = Qt.binding(() => slotRow.modelData.assignedGrantId === choice.modelData.grantId);
                                                    if (routine.blocked || !slotRow.manual) return;
                                                    if (!slotRow.choices.some(candidate => candidate.grantId === modelData.grantId)) return;
                                                    root.assignRequested(routine.modelData.id, slotRow.modelData.id, modelData.grantId);
                                                }
                                            }
                                            onObjectAdded: (index, object) => choicesMenu.insertItem(index, object)
                                            onObjectRemoved: (index, object) => choicesMenu.removeItem(object)
                                        }
                                    }
                                }
                                Label {
                                    visible: slotRow.modelData.selectionMode === AttacksPanel.AllMatching
                                    width: parent.width
                                    text: qsTr("Inclusi automaticamente")
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 12
                                    font.italic: true
                                    color: "#666666"
                                }
                                InfoDetails {
                                    visible: routineInfo.checked && rows.length > 0
                                    width: parent.width
                                    framed: false
                                    padding: 0
                                    rows: (slotRow.modelData.failureReasons ?? []).map(reason => ({kind: InfoDetails.Note, text: reason}))
                                }
                            }
                        }
                        Label {
                            visible: !!routine.modelData.calculatedAttacks?.length
                            text: qsTr("Attacchi generati")
                            font.pixelSize: 14
                            font.bold: true
                            color: "#253947"
                        }
                        Repeater {
                            model: routine.modelData.calculatedAttacks ?? []
                            Rectangle {
                                id: attackRow
                                required property var modelData
                                required property int index
                                objectName: "attack_" + routine.modelData.id + "_" + index
                                width: slotsContent.width
                                implicitHeight: attackContent.height + 20
                                color: "#ffffff"
                                border.color: "#d8e0e5"
                                radius: 4
                                Column {
                                    id: attackContent
                                    readonly property var strike: attackRow.modelData.strike
                                    readonly property var confirmationSituational: root.situationalBonuses(strike?.criticalConfirmationModifiers)
                                    x: 10; y: 10
                                    width: parent.width - 20
                                    spacing: 5
                                    Column {
                                        width: parent.width
                                        spacing: 1
                                        Row {
                                            width: parent.width
                                            spacing: 8
                                            Label {
                                                width: parent.width - bonusLabel.width - 8
                                                text: (attackRow.index + 1) + ". " + attackRow.modelData.strikeName
                                                textFormat: Text.PlainText
                                                wrapMode: Text.Wrap
                                                font.pixelSize: 14
                                                color: "#253947"
                                            }
                                            Label {
                                                id: bonusLabel
                                                objectName: "attackBonus_" + routine.modelData.id + "_" + attackRow.index
                                                width: Math.min(implicitWidth, parent.width / 2)
                                                text: root.attackBonus(attackRow.modelData)
                                                wrapMode: Text.Wrap
                                                font.pixelSize: 20
                                                font.bold: true
                                                color: "#253947"
                                                Accessible.name: qsTr("Tiro per colpire: ") + text
                                            }
                                        }
                                        SituationalLines {
                                            objectName: "attackRollSituational"
                                            width: parent.width
                                            entries: root.situationalBonuses(attackContent.strike?.attackModifiers)
                                        }
                                    }
                                    Label {
                                        width: parent.width
                                        text: qsTr("Danni: ") + root.damageSummary(attackRow.modelData.normalDamage)
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 13
                                        color: "#555555"
                                    }
                                    SituationalLines {
                                        objectName: "attackDamageSituational"
                                        width: parent.width
                                        entries: root.situationalBonuses(attackContent.strike?.damageModifiers)
                                    }
                                    Label {
                                        width: parent.width
                                        text: qsTr("Critico: ") + root.criticalSummary(attackRow.modelData)
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 13
                                        color: "#555555"
                                    }
                                    Label {
                                        objectName: "confirmation_" + routine.modelData.id + "_" + attackRow.index
                                        visible: root.hasDifferentConfirmation(attackRow.modelData) || attackContent.confirmationSituational.length > 0
                                        width: parent.width
                                        text: qsTr("Conferma critico: ") + root.confirmationBonus(attackRow.modelData)
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 13
                                        color: "#555555"
                                    }
                                    SituationalLines {
                                        objectName: "attackConfirmationSituational"
                                        width: parent.width
                                        entries: attackContent.confirmationSituational
                                    }
                                    Label {
                                        objectName: "criticalDamage_" + routine.modelData.id + "_" + attackRow.index
                                        width: parent.width
                                        text: qsTr("Danni critici: ") + root.damageSummary(attackRow.modelData.criticalDamage)
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 13
                                        color: "#555555"
                                    }
                                    AttackDetails {
                                        objectName: "attackDetails_" + routine.modelData.id + "_" + attackRow.index
                                        visible: routineInfo.checked
                                        width: parent.width
                                        attack: attackRow.modelData
                                        baseAttackBonus: root.baseAttackBonus
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
