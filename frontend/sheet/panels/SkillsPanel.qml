pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Item {
    id: root
    required property var skills
    property var skillCategories: []
    property bool readOnly: false
    property var spentRanks: null
    property var maxRanks: null
    enum SpecializationType { Craft = 2, Perform = 21, Profession = 26 }
    signal specializationAddRequested(int skillType, string name)
    signal ranksChangeRequested(string resourceName, int ranks)
    signal specializationRemoveRequested(int skillType, string specializationId)
    signal classSkillChangeRequested(int skillType, bool isClassSkill)
    property var expandedTypes: []
    property var pinnedResources: []
    onSkillsChanged: {
        const present = new Set(Array.from(skills ?? []).map(skill => skill.resourceName));
        pinnedResources = (pinnedResources ?? []).filter(resource => present.has(resource));
    }
    implicitWidth: 520
    implicitHeight: table.height

    function setExpanded(type, expanded) {
        expandedTypes = expanded ? [...new Set([...expandedTypes, type])] : expandedTypes.filter(entry => entry !== type);
    }

    function categoryName(type) {
        switch (type) {
        case SkillsPanel.Craft: return qsTr("Artigianato");
        case SkillsPanel.Perform: return qsTr("Intrattenere");
        case SkillsPanel.Profession: return qsTr("Professione");
        default: return "";
        }
    }

    readonly property var sortedSkills: {
        const rows = Array.from(skills ?? []).filter(skill => !categoryName(skill.type) || skill.specializationId)
            .map(skill => Object.assign({}, skill, {
                name: categoryName(skill.type) ? categoryName(skill.type) + " (" + skill.name + ")" : skill.name
            }));
        for (const type of [SkillsPanel.Craft, SkillsPanel.Perform, SkillsPanel.Profession]) {
            const category = skillCategories.find(entry => entry.type === type)
                ?? Array.from(skills ?? []).find(skill => skill.type === type && !skill.specializationId);
            rows.push({ type: type, name: categoryName(type), resourceName: "specializationCategory" + type,
                category: true, classSkill: category?.classSkill ?? false, customClassSkill: category?.customClassSkill ?? false });
        }
        return rows.sort((a, b) => a.name.localeCompare(b.name, "it") || a.resourceName.localeCompare(b.resourceName));
    }

    QtObject {
        id: rowState
        readonly property var signatures: new Map()
    }
    ListModel {
        id: skillRows
        dynamicRoles: true
    }

    onSortedSkillsChanged: if (skillRows && rowState) syncRows()
    Component.onCompleted: syncRows()

    function syncRows() {
        const nextRows = sortedSkills;
        const nextKeys = new Set(nextRows.map(skill => skill.resourceName));
        for (let index = skillRows.count - 1; index >= 0; --index) {
            const key = skillRows.get(index).resourceName;
            if (!nextKeys.has(key)) {
                skillRows.remove(index);
                rowState.signatures.delete(key);
            }
        }

        for (let index = 0; index < nextRows.length; ++index) {
            const skill = nextRows[index];
            const key = skill.resourceName;
            const signature = JSON.stringify(skill);
            let previous = index;
            while (previous < skillRows.count && skillRows.get(previous).resourceName !== key) ++previous;
            if (previous === skillRows.count) {
                skillRows.insert(index, {resourceName: key, skillData: skill});
            } else {
                if (previous !== index) skillRows.move(previous, index, 1);
                if (rowState.signatures.get(key) !== signature) skillRows.setProperty(index, "skillData", skill);
            }
            rowState.signatures.set(key, signature);
        }
    }

    function scoreText(skill) {
        if (!skill.usable) return "—";
        const values = Array.from(skill.abilityOptions ?? []).map(option => option.totalValue);
        return values.length ? [...new Set(values)].map(value => value >= 0 ? "+" + value : String(value)).join(" / ") : "—";
    }
    function situationalBonuses(skill) {
        const byOption = (skill.abilityOptions ?? []).map(option =>
            new Map((option.modifiers?.conditionalTotals ?? []).map(entry => [entry.condition, entry.value])));
        const conditions = new Set();
        for (const totals of byOption)
            for (const condition of totals.keys()) conditions.add(condition);
        return Array.from(conditions, condition => ({
            condition: condition,
            values: [...new Set(byOption.map(totals => totals.get(condition) ?? 0))]
        }));
    }

    Column {
        id: table
        width: Math.min(root.width, root.implicitWidth)
        spacing: 2
        Rectangle {
            width: table.width
            height: 34
            color: "#181818"
            Row {
                anchors.fill: parent
                Label { width: 44; height: parent.height; text: qsTr("Classe"); color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Label { width: table.width - 202; height: parent.height; text: qsTr("Abilità"); color: "white"; font.bold: true; verticalAlignment: Text.AlignVCenter }
                Label { width: 80; height: parent.height; text: qsTr("Punteggio"); color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Label { width: 50; height: parent.height; text: qsTr("Gradi"); color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
        Repeater {
            model: skillRows
            Rectangle {
                id: row
                required property var skillData
                required property int index
                readonly property var modelData: skillData
                objectName: modelData.resourceName
                visible: !!modelData.category || !root.categoryName(modelData.type) || root.expandedTypes.includes(modelData.type) || root.pinnedResources.includes(modelData.resourceName)
                width: table.width
                readonly property real mainHeight: Math.max(40, skillName.implicitHeight + 16, score.implicitHeight + 16)
                height: mainHeight + (situational.visible ? situational.height + 4 : 0) + (details.visible ? details.height : 0)
                color: modelData.category ? "#e3e3e3" : (index % 2 ? "#f2f2f2" : "#fafafa")
                Rectangle {
                    width: parent.width
                    height: 1
                    y: row.mainHeight - 1
                    color: "#dedede"
                }
                ToolButton {
                    objectName: "removeSpecialization"
                    width: 22
                    height: row.mainHeight
                    visible: !root.readOnly && !!row.modelData.specializationId && row.modelData.custom === true
                    enabled: visible
                    text: "×"
                    Accessible.name: qsTr("Rimuovi ") + row.modelData.name
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: root.specializationRemoveRequested(row.modelData.type, row.modelData.specializationId)
                }
                ToolButton {
                    id: pin
                    objectName: "pinSpecialization"
                    padding: 0
                    x: 22
                    width: 22
                    height: row.mainHeight
                    visible: !!row.modelData.specializationId
                    checked: root.pinnedResources.includes(row.modelData.resourceName)
                    background: Rectangle {
                        radius: 3
                        color: pin.hovered || pin.visualFocus ? "#e4e4e4" : "transparent"
                    }
                    contentItem: Item {
                        Canvas {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            readonly property bool pinned: pin.checked
                            onPinnedChanged: requestPaint()
                            onPaint: {
                                const ctx = getContext("2d");
                                ctx.clearRect(0, 0, width, height);
                                ctx.strokeStyle = pinned ? "#181818" : "#666666";
                                ctx.fillStyle = "#181818";
                                ctx.lineWidth = 1.3;
                                ctx.lineJoin = "round";
                                ctx.lineCap = "round";
                                ctx.beginPath();
                                ctx.moveTo(5, 2.5);
                                ctx.lineTo(11, 2.5);
                                ctx.lineTo(10, 6);
                                ctx.lineTo(12.5, 9.5);
                                ctx.lineTo(3.5, 9.5);
                                ctx.lineTo(6, 6);
                                ctx.closePath();
                                if (pinned) ctx.fill();
                                ctx.stroke();
                                ctx.beginPath();
                                ctx.moveTo(8, 9.5);
                                ctx.lineTo(8, 14);
                                ctx.stroke();
                            }
                        }
                    }
                    Accessible.name: (checked ? qsTr("Non mantenere visibile ") : qsTr("Mantieni visibile ")) + row.modelData.name
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: {
                        const resource = row.modelData.resourceName;
                        root.pinnedResources = checked ? root.pinnedResources.filter(entry => entry !== resource)
                            : [...root.pinnedResources, resource];
                    }
                }
                CheckBox {
                    id: classSkill
                    objectName: "classSkill"
                    width: 44
                    height: row.mainHeight
                    visible: !!row.modelData.category || !root.categoryName(row.modelData.type)
                    checked: row.modelData.classSkill ?? false
                    checkable: !root.readOnly && (!checked || row.modelData.customClassSkill === true)
                    nextCheckState: () => classSkill.checkState
                    focusPolicy: checkable || activeFocus ? Qt.StrongFocus : Qt.NoFocus
                    onClicked: {
                        if (checkable) root.classSkillChangeRequested(row.modelData.type, !checked);
                    }
                    Accessible.name: row.modelData.name + qsTr(" — abilità di classe")
                }
                RowLayout {
                    visible: !!row.modelData.category
                    x: 44
                    width: parent.width - 48
                    height: row.mainHeight
                    spacing: 8
                    Button {
                        objectName: "specializationToggle"
                        text: (root.expandedTypes.includes(row.modelData.type) ? "▾ " : "▸ ") + row.modelData.name
                        flat: true
                        font.bold: true
                        font.pixelSize: 15
                        Accessible.name: row.modelData.name + qsTr(" — mostra specializzazioni")
                        onClicked: root.setExpanded(row.modelData.type, !root.expandedTypes.includes(row.modelData.type))
                    }
                    TextField {
                        ContextMenu.menu: null
                        objectName: "specializationInput"
                        visible: !root.readOnly
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        placeholderText: qsTr("Nuova specializzazione")
                        Accessible.name: row.modelData.name + qsTr(" — nuova specializzazione")
                        selectByMouse: true
                        onAccepted: {
                            const name = text.trim();
                            focus = false;
                            if (name.length > 0) {
                                root.setExpanded(row.modelData.type, true);
                                root.specializationAddRequested(row.modelData.type, name);
                            }
                        }
                    }
                }
                Row {
                    x: 44
                    width: parent.width - 44
                    height: row.mainHeight
                    visible: !row.modelData.category
                    Label {
                        id: skillName
                        objectName: "skillName"
                        width: table.width - 202
                        leftPadding: row.modelData.specializationId ? 20 : 0
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.name
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: "#181818"
                        font.weight: row.modelData.specializationId ? Font.Normal : Font.Medium
                        font.pixelSize: 15
                    }
                    Label {
                        id: score
                        objectName: "score"
                        width: 80
                        height: row.mainHeight - 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.category ? "" : root.scoreText(row.modelData)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 18
                        font.bold: true
                        wrapMode: Text.Wrap
                        color: "#181818"
                        background: Rectangle {
                            x: 6
                            width: parent.width - 12
                            height: parent.height
                            color: "white"
                            border.color: "#bbbbbb"
                        }
                    }
                    Item {
                        id: ranksCell
                        objectName: "ranksCell"
                        width: 50
                        height: parent.height
                        readonly property var savedRanks: row.modelData.ranks
                        onSavedRanksChanged: if (ranksInput && !ranksInput.activeFocus) resetDraft()
                        function resetDraft() { ranksInput.text = String(savedRanks ?? ""); }
                        function submit() {
                            const value = Number(ranksInput.text);
                            const valid = /^\d+$/.test(ranksInput.text) && Number.isInteger(value) && value >= 0 && value <= 2147483647;
                            const changed = valid && value !== row.modelData.ranks;
                            const resource = row.modelData.resourceName;
                            if (changed) root.ranksChangeRequested(resource, value);
                            resetDraft();
                        }
                        HoverHandler {
                            id: ranksHover
                            onHoveredChanged: {
                                if (hovered && !ranksInput.activeFocus) ranksCell.resetDraft();
                            }
                        }
                        Label {
                            objectName: "effectiveRanks"
                            anchors.fill: parent
                            visible: root.readOnly || (!ranksHover.hovered && !ranksInput.activeFocus)
                            text: row.modelData.category ? "" : String(row.modelData.effectiveRanks ?? row.modelData.ranks)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#181818"
                            font.pixelSize: 16
                            Accessible.name: row.modelData.name + qsTr(" — gradi effettivi")
                        }
                        TextField {
                            ContextMenu.menu: null
                            id: ranksInput
                            objectName: "ranks"
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            visible: !root.readOnly && (ranksHover.hovered || activeFocus)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#181818"
                            font.pixelSize: 16
                            background: Rectangle { color: "#fff9e9"; border.color: "#777777" }
                            selectByMouse: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            Accessible.name: row.modelData.name + qsTr(" — gradi assegnati")
                            Component.onCompleted: ranksCell.resetDraft()
                            onAccepted: { ranksCell.submit(); focus = false; }
                            onEditingFinished: ranksCell.submit()
                            Keys.onEscapePressed: { ranksCell.resetDraft(); focus = false; }
                        }
                    }
                    InfoButton {
                        id: infoButton
                        objectName: "skillInfo"
                        width: 28
                        height: row.mainHeight
                        padding: 0
                        text: "ⓘ"
                        checkable: true
                        Accessible.name: qsTr("Dettaglio di ") + row.modelData.name
                    }
                }
                SituationalBonusCards {
                    id: situational
                    x: skillName.parent.x + skillName.x + skillName.leftPadding
                    y: row.mainHeight + 2
                    width: Math.max(0, score.parent.x + score.x + score.background.x + score.background.width - x)
                    entries: row.modelData.category ? [] : root.situationalBonuses(row.modelData)
                }
                SkillDetails {
                    id: details
                    objectName: "skillDetails"
                    y: row.mainHeight + (situational.visible ? situational.height + 4 : 0)
                    width: row.width
                    visible: !row.modelData.category && infoButton.checked
                    skill: row.modelData
                }
            }
        }
        Label {
            objectName: "rankBudget"
            width: table.width
            padding: 10
            text: qsTr("Gradi spesi / disponibili: ") + (root.spentRanks ?? "—") + " / " + (root.maxRanks ?? "—")
            font.bold: true
            color: "#181818"
            background: Rectangle { color: "#e3e3e3" }
            wrapMode: Text.Wrap
        }
    }
}
