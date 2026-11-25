pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ConditionIcons.js" as ConditionIcons

Item {
    id: root
    required property var identity
    required property string notes
    property bool combatEnabled: false
    property var abilityScoresData: ({})
    property var skillScoresData: ({})
    property var savingThrowsData: ({})
    property var armorClassData: ({})
    property var hitPointsData: ({})
    property var movementData: ({})
    property var defensesData: ({})
    property var initiativeData: ({})
    property var activeConditionsData: ({})
    property var actionsData: ({})
    property var attacksData: ({})
    property var baseAttackBonusData: ({})
    property var combatManeuversData: ({})
    property var attackRoutinesData: ({})
    property Component inventoryTabPage
    property Component raceTabPage
    property Component classesTabPage
    property Component featsTabPage
    property Component spellsTabPage
    property Component enhancementsTabPage
    property Component conditionsTabPage
    readonly property int inventoryTabIndex: combatEnabled ? 3 : 2
    readonly property int raceTabIndex: inventoryTabIndex + (inventoryTabPage !== null ? 1 : 0)
    readonly property int classesTabIndex: raceTabIndex + (raceTabPage !== null ? 1 : 0)
    readonly property int featsTabIndex: classesTabIndex + (classesTabPage !== null ? 1 : 0)
    readonly property int spellsTabIndex: featsTabIndex + (featsTabPage !== null ? 1 : 0)
    readonly property int enhancementsTabIndex: spellsTabIndex + (spellsTabPage !== null ? 1 : 0)
    readonly property int conditionsTabIndex: enhancementsTabIndex + (enhancementsTabPage !== null ? 1 : 0)
    readonly property alias spellsTabItem: spellsTabLoader.item
    property alias currentTab: tabs.currentIndex
    property string identityErrorMessage: ""
    function tabForChoice(type) {
        switch (type) {
        case PendingChoicesPanel.Alignment: return 0;
        case PendingChoicesPanel.SkillRanks: return 1;
        case PendingChoicesPanel.Race:
        case PendingChoicesPanel.RacialOption: return raceTabPage ? raceTabIndex : -1;
        case PendingChoicesPanel.ClassOption:
        case PendingChoicesPanel.ClassHitDie:
        case PendingChoicesPanel.FavoredClass:
        case PendingChoicesPanel.FavoredClassBonus: return classesTabPage ? classesTabIndex : -1;
        case PendingChoicesPanel.FeatSlot:
        case PendingChoicesPanel.Language: return featsTabPage ? featsTabIndex : -1;
        case PendingChoicesPanel.PreparedSpell:
        case PendingChoicesPanel.FlexiblePreparation:
        case PendingChoicesPanel.Repertoire: return spellsTabPage ? spellsTabIndex : -1;
        default: return -1;
        }
    }
    function openChoiceTab(type) {
        const target = tabForChoice(type);
        if (target < 0) return;
        currentTab = target;
        const flickable = contentScroll.contentItem;
        flickable.contentY = flickable.originY;
    }
    signal notesChangeRequested(string notes)
    signal identityFieldChangeRequested(string field, var value)

    property bool hitPointEditingEnabled: false
    property bool statisticsEditingEnabled: false
    signal temporaryAddRequested(string source, int amount)
    signal temporaryRemoveRequested(string source)
    signal damageRequested(int amount, bool nonLethal)
    signal healRequested(int amount)
    signal abilityBaseChangeRequested(int abilityType, int value)
    signal skillRanksChangeRequested(string resourceName, int ranks)
    signal skillSpecializationAddRequested(int skillType, string name)
    signal skillSpecializationRemoveRequested(int skillType, string specializationId)
    signal classSkillChangeRequested(int skillType, bool isClassSkill)
    signal attackRoutineAddRequested(string routineId)
    signal attackRoutineRemoveRequested(string attackId)
    signal attackRoutineRenameRequested(string attackId, string name)
    signal attackRoutineReorderRequested(string attackId, var beforeAttackId)
    signal attackStrikeAssignRequested(string attackId, string slotId, string strikeGrantId)

    component CachedTab: Loader {
        property bool selected: false
        active: false
        visible: selected
        onSelectedChanged: if (selected && !active) active = true
        Component.onCompleted: if (selected && !active) active = true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        TabBar {
            id: tabs
            objectName: "sheetTabs"
            Layout.fillWidth: true
            TabButton { text: qsTr("Identità") }
            TabButton { text: qsTr("Home") }
            Repeater {
                model: root.combatEnabled ? 1 : 0
                TabButton { text: qsTr("Combattimento") }
            }
            Repeater {
                model: root.inventoryTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Inventario") }
            }
            Repeater {
                model: root.raceTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Razza") }
            }
            Repeater {
                model: root.classesTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Classi") }
            }
            Repeater {
                model: root.featsTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Capacità") }
            }
            Repeater {
                model: root.spellsTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Incantesimi") }
            }
            Repeater {
                model: root.enhancementsTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Potenziamenti") }
            }
            Repeater {
                model: root.conditionsTabPage !== null ? 1 : 0
                TabButton { text: qsTr("Condizioni") }
            }
        }
        ScrollView {
            id: contentScroll
            objectName: "sheetContentScroll"
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            Binding {
                target: contentScroll.contentItem
                property: "boundsBehavior"
                value: Flickable.StopAtBounds
            }

            Column {
                width: contentScroll.availableWidth
                GridLayout {
                    id: identityLayout
                    visible: tabs.currentIndex === 0
                    width: contentScroll.availableWidth
                    columns: width >= 960 ? 2 : 1
                    columnSpacing: 16
                    rowSpacing: 16
                    IdentityPanel {
                        objectName: "sheetIdentityPanel"
                        Layout.fillWidth: true
                        Layout.preferredWidth: 680
                        Layout.alignment: Qt.AlignTop
                        showTitle: false
                        commitOnEnter: true
                        errorMessage: root.identityErrorMessage
                        identity: root.identity
                        onIdentityFieldChangeRequested: (field, value) => root.identityFieldChangeRequested(field, value)
                        background: Item {
                            Rectangle {
                                objectName: "identityDivider"
                                x: parent.width + (identityLayout.columnSpacing - width) / 2
                                width: 1
                                height: identityLayout.height
                                visible: identityLayout.columns === 2
                                color: "#d2dbe1"
                            }
                        }
                    }
                    NotesPanel {
                        objectName: "sheetNotesPanel"
                        Layout.fillWidth: true
                        Layout.preferredWidth: 340
                        Layout.alignment: Qt.AlignTop
                        editorHeight: identityLayout.columns === 2 ? Math.max(320, contentScroll.availableHeight - 64) : 320
                        notes: root.notes
                        onNotesChangeRequested: notes => root.notesChangeRequested(notes)
                    }
                }
                CachedTab {
                    width: contentScroll.availableWidth
                    selected: tabs.currentIndex === 1
                    sourceComponent: GridLayout {
                        id: homeLayout
                        objectName: "sheetHomeLayout"
                        columns: width >= 880 ? 2 : 1
                        columnSpacing: 24
                        rowSpacing: 16
                        GridLayout {
                            id: homeSummary
                            objectName: "sheetHomeSummary"
                            columns: homeLayout.width >= 1040 ? 2 : 1
                            columnSpacing: 16
                            rowSpacing: 16
                            Layout.fillWidth: true
                            Layout.preferredWidth: columns === 2 ? 600 : 360
                            Layout.alignment: Qt.AlignTop
                            Column {
                                id: homeAbilities
                                objectName: "sheetHomeAbilities"
                                Layout.fillWidth: true
                                Layout.preferredWidth: 340
                                Layout.alignment: Qt.AlignTop
                                spacing: 12
                                AbilityScoresPanel {
                                    width: parent.width
                                    abilities: root.abilityScoresData.abilities ?? []
                                    readOnly: !root.statisticsEditingEnabled
                                    onBaseValueChangeRequested: (type, value) => root.abilityBaseChangeRequested(type, value)
                                }
                            }
                            Column {
                                id: homeChecks
                                objectName: "sheetHomeChecks"
                                Layout.fillWidth: true
                                Layout.preferredWidth: 260
                                Layout.alignment: Qt.AlignTop
                                readonly property bool sideBySide: homeSummary.columns === 2
                                readonly property real freeHeight: Math.max(0, homeAbilities.height - homeInitiative.height - homeSavingThrows.height)
                                topPadding: sideBySide ? (freeHeight - spacing) / 2 : 0
                                bottomPadding: topPadding
                                spacing: sideBySide ? freeHeight * 0.30 : 44
                                InitiativePanel { id: homeInitiative; width: parent.width; initiative: root.initiativeData.initiative }
                                SavingThrowsPanel { id: homeSavingThrows; width: parent.width; savingThrows: root.savingThrowsData.savingThrows ?? [] }
                            }
                            ArmorClassPanel {
                                objectName: "sheetHomeArmorClass"
                                Layout.columnSpan: homeSummary.columns
                                Layout.fillWidth: true
                                armorClass: root.armorClassData.armorClass
                            }
                            HitPointsPanel {
                                objectName: "sheetHomeHitPoints"
                                Layout.columnSpan: homeSummary.columns
                                maximumDetailsWidth: width
                                onTemporaryAddRequested: (source, amount) => root.temporaryAddRequested(source, amount)
                                onTemporaryRemoveRequested: source => root.temporaryRemoveRequested(source)
                                onDamageRequested: (amount, lethality) => root.damageRequested(amount, lethality === HitPointsPanel.NonLethal)
                                onHealRequested: amount => root.healRequested(amount)
                                Layout.fillWidth: true
                                hitPoints: root.hitPointsData.hitPoints
                                readOnly: !root.hitPointEditingEnabled
                                belowHeader: Component {
                                    Column {
                                        spacing: 8
                                        DefensesPanel { width: parent.width; defenses: root.defensesData.defenses; presentation: DefensesPanel.Tags }
                                        DefensesPanel { width: parent.width; defenses: root.defensesData.defenses; presentation: DefensesPanel.Shields }
                                    }
                                }
                            }
                            MovementPanel {
                                Layout.columnSpan: homeSummary.columns
                                Layout.fillWidth: true
                                movement: root.movementData.movement
                                showHeading: false
                                visible: grants.length > 0
                            }
                            Column {
                                id: activeConditions
                                objectName: "sheetHomeActiveConditions"
                                Layout.columnSpan: homeSummary.columns
                                Layout.fillWidth: true
                                readonly property var entries: root.activeConditionsData.activeConditions ?? []
                                property string selectedId: ""
                                visible: entries.length > 0
                                spacing: 6

                                Flow {
                                    width: parent.width
                                    spacing: 8
                                    Repeater {
                                        model: activeConditions.entries
                                        Column {
                                            required property var modelData
                                            width: Math.min(activeConditions.width, Math.max(condition.checked ? 320 : 148,
                                                Math.ceil(conditionTextMetrics.advanceWidth) + condition.leftPadding + condition.rightPadding
                                                + condition.spacing + condition.conditionIconSize))
                                            spacing: 4
                                            ToolButton {
                                                id: condition
                                                readonly property int conditionIconSize: (modelData.iconId ?? modelData.id) === "dazed" ? 27 : 24
                                                objectName: "homeCondition_" + modelData.id
                                                text: modelData.name
                                                checked: activeConditions.selectedId === modelData.id
                                                width: parent.width
                                                implicitHeight: 42
                                                leftPadding: 14
                                                rightPadding: 12
                                                spacing: 8
                                                font.pixelSize: 15
                                                font.bold: true
                                                icon.source: ConditionIcons.source(modelData.iconId ?? modelData.id)
                                                icon.width: conditionIconSize
                                                icon.height: conditionIconSize
                                                icon.color: checked ? "#ffffff" : "#181818"
                                                palette.buttonText: icon.color
                                                TextMetrics { id: conditionTextMetrics; font: condition.font; text: condition.text }
                                                onClicked: activeConditions.selectedId = checked ? "" : modelData.id
                                                background: Rectangle {
                                                    radius: 5
                                                    color: condition.checked ? "#181818" : condition.hovered ? "#e5e5e5" : "#f8f8f8"
                                                    border.color: condition.checked ? "#181818" : "#999999"
                                                }
                                            }
                                            InfoDetails {
                                                objectName: "sheetHomeConditionDetails_" + modelData.id
                                                width: parent.width
                                                visible: condition.checked
                                                rows: [{kind: InfoDetails.Heading, text: modelData.name},
                                                    ...(modelData.effects ?? []).map(effect => ({kind: InfoDetails.Entry, text: effect}))]
                                            }
                                        }
                                    }
                                }
                            }
                            Label {
                                Layout.columnSpan: homeSummary.columns
                                Layout.fillWidth: true
                                visible: root.identityErrorMessage.length > 0
                                text: root.identityErrorMessage
                                color: "#a32626"
                                wrapMode: Text.Wrap
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 520
                            Layout.alignment: Qt.AlignTop
                            implicitHeight: homeSkillsLoader.status === Loader.Ready ? homeSkillsLoader.height : skillsLoading.implicitHeight

                            Timer {
                                interval: 0
                                running: homeLayout.visible && !homeSkillsLoader.active
                                onTriggered: homeSkillsLoader.active = true
                            }
                            Label {
                                id: skillsLoading
                                objectName: "sheetHomeSkillsLoading"
                                width: parent.width
                                visible: homeSkillsLoader.status !== Loader.Ready
                                text: qsTr("Caricamento abilità…")
                                color: "#555555"
                                wrapMode: Text.Wrap
                            }
                            Loader {
                                id: homeSkillsLoader
                                objectName: "sheetHomeSkillsLoader"
                                width: parent.width
                                active: false
                                asynchronous: true
                                visible: status === Loader.Ready
                                sourceComponent: SkillsPanel {
                                    objectName: "sheetHomeSkills"
                                    skills: root.skillScoresData.skills ?? []
                                    skillCategories: root.skillScoresData.skillCategories ?? []
                                    spentRanks: root.skillScoresData.spentRanks
                                    maxRanks: root.skillScoresData.maxRanks
                                    readOnly: !root.statisticsEditingEnabled
                                    onRanksChangeRequested: (resourceName, ranks) => root.skillRanksChangeRequested(resourceName, ranks)
                                    onSpecializationAddRequested: (type, name) => root.skillSpecializationAddRequested(type, name)
                                    onSpecializationRemoveRequested: (type, id) => root.skillSpecializationRemoveRequested(type, id)
                                    onClassSkillChangeRequested: (type, enabled) => root.classSkillChangeRequested(type, enabled)
                                }
                            }
                        }
                    }
                }
                CachedTab {
                    width: contentScroll.availableWidth
                    selected: root.combatEnabled && tabs.currentIndex === 2
                    sourceComponent: root.combatEnabled ? combatPage : null
                    Component {
                        id: combatPage
                        Column {
                            Label {
                                width: parent.width
                                visible: root.identityErrorMessage.length > 0
                                text: root.identityErrorMessage
                                color: "#a32626"
                                wrapMode: Text.Wrap
                            }
                            GridLayout {
                                id: combatLayout
                                objectName: "sheetCombatLayout"
                                width: parent.width
                                columns: width >= 1040 ? 3 : width >= 820 ? 2 : 1
                                columnSpacing: 24
                                rowSpacing: 16
                                readonly property real equalColumnWidth: (width - 2 * columnSpacing) / 3
                                AttacksPanel {
                                    objectName: "sheetCombatAttacks"
                                    prominentHeading: true
                                    Layout.row: 0
                                    Layout.column: 0
                                    Layout.rowSpan: combatLayout.columns === 2 ? 2 : 1
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: combatLayout.columns === 3 ? combatLayout.equalColumnWidth : implicitWidth
                                    Layout.alignment: Qt.AlignTop
                                    attacks: root.attacksData?.attacks ?? null
                                    attackRoutines: root.attackRoutinesData?.attackRoutines ?? null
                                    actions: root.actionsData?.actions ?? null
                                    baseAttackBonus: root.baseAttackBonusData?.baseAttackBonus ?? null
                                    onAddRequested: routineId => root.attackRoutineAddRequested(routineId)
                                    onRemoveRequested: attackId => root.attackRoutineRemoveRequested(attackId)
                                    onRenameRequested: (attackId, name) => root.attackRoutineRenameRequested(attackId, name)
                                    onReorderRequested: (attackId, beforeAttackId) => root.attackRoutineReorderRequested(attackId, beforeAttackId)
                                    onAssignRequested: (attackId, slotId, strikeGrantId) => root.attackStrikeAssignRequested(attackId, slotId, strikeGrantId)
                                }
                                CombatManeuversPanel {
                                    objectName: "sheetCombatManeuvers"
                                    prominentHeading: true
                                    Layout.row: combatLayout.columns === 1 ? 1 : 0
                                    Layout.column: combatLayout.columns === 1 ? 0 : 1
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: combatLayout.columns === 3 ? combatLayout.equalColumnWidth : 340
                                    Layout.alignment: Qt.AlignTop
                                    combatManeuvers: root.combatManeuversData?.combatManeuvers ?? null
                                }
                                ActionsPanel {
                                    objectName: "sheetCombatActions"
                                    prominentHeading: true
                                    Layout.row: 3 - combatLayout.columns
                                    Layout.column: combatLayout.columns - 1
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: combatLayout.columns === 3 ? combatLayout.equalColumnWidth : 270
                                    Layout.alignment: Qt.AlignTop
                                    actions: root.actionsData?.listedActions ?? root.actionsData?.actions ?? null
                                    attackRoutines: root.attackRoutinesData?.listedActionRoutines ?? root.attackRoutinesData?.attackRoutines ?? null
                                }
                            }
                        }
                    }
                }
                CachedTab {
                    objectName: "sheetInventoryTab"
                    width: contentScroll.availableWidth
                    selected: root.inventoryTabPage !== null && tabs.currentIndex === root.inventoryTabIndex
                    sourceComponent: root.inventoryTabPage
                }
                CachedTab {
                    objectName: "sheetRaceTab"
                    width: contentScroll.availableWidth
                    selected: root.raceTabPage !== null && tabs.currentIndex === root.raceTabIndex
                    sourceComponent: root.raceTabPage
                }
                CachedTab {
                    objectName: "sheetClassesTab"
                    width: contentScroll.availableWidth
                    selected: root.classesTabPage !== null && tabs.currentIndex === root.classesTabIndex
                    sourceComponent: root.classesTabPage
                }
                CachedTab {
                    width: contentScroll.availableWidth
                    selected: root.featsTabPage !== null && tabs.currentIndex === root.featsTabIndex
                    sourceComponent: root.featsTabPage
                }
                CachedTab {
                    id: spellsTabLoader
                    width: contentScroll.availableWidth
                    selected: root.spellsTabPage !== null && tabs.currentIndex === root.spellsTabIndex
                    sourceComponent: root.spellsTabPage
                }
                CachedTab {
                    width: contentScroll.availableWidth
                    selected: root.enhancementsTabPage !== null && tabs.currentIndex === root.enhancementsTabIndex
                    sourceComponent: root.enhancementsTabPage
                }
                CachedTab {
                    width: contentScroll.availableWidth
                    selected: root.conditionsTabPage !== null && tabs.currentIndex === root.conditionsTabIndex
                    sourceComponent: root.conditionsTabPage
                }
            }
        }
    }
}
