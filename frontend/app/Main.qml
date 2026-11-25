pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App
import Golarion.Sheet

ApplicationWindow {
    id: window

    required property SessionController sessionController
    required property SheetController sheetController
    readonly property bool inSession: sessionController.activeName.length > 0
    readonly property bool hosting: sessionController.hostingPort > 0
    readonly property HostedCampaignPage hostedPage: navigation.currentItem as HostedCampaignPage

    width: 1100
    height: 760
    minimumWidth: 360
    minimumHeight: 240
    visible: true
    title: qsTr("Golarion Helper")
    color: "#f7f8fa"
    font.pixelSize: 14
    palette.windowText: "#222831"
    palette.buttonText: "#222831"
    palette.button: "#eef1f4"
    palette.highlight: "#315b83"

    header: ToolBar {
        padding: 12
        leftPadding: 20
        rightPadding: 20

        background: Rectangle {
            color: "#ffffff"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#e1e5ea"
            }
        }

        contentItem: GridLayout {
            columns: window.sessionController.hostedCampaignOpen && window.width < 540 ? 1 : 2
            columnSpacing: 16
            rowSpacing: 6

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: window.sessionController.hostedCampaignOpen ? 40 : 0
                spacing: 12

                Button {
                    Layout.minimumHeight: 34
                    leftPadding: 14
                    rightPadding: 14
                    visible: navigation.depth > 1 &&
                             (!window.inSession || (window.sessionController.hostedCampaignOpen && !window.hosting))
                    enabled: !window.sessionController.busy && !window.sheetController.opening
                    text: qsTr("Indietro")
                    onClicked: {
                        window.sessionController.clearError()
                        if (window.sessionController.hostedCampaignOpen)
                            window.sessionController.closeHostedCampaign()
                        else
                            navigation.pop(navigation.get(navigation.depth - 2), StackView.Immediate)
                    }
                }


                Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    visible: window.inSession
                    text: window.sessionController.activeName
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                TextField {
                    id: displayNameField
                    Layout.preferredWidth: 180
                    visible: !window.inSession
                    placeholderText: qsTr("Nome")
                    Accessible.name: qsTr("Nome visualizzato")
                    horizontalAlignment: TextInput.AlignRight
                    selectByMouse: true
                    palette.base: "#ffffff"
                    palette.text: window.palette.windowText
                }

                Label {
                    visible: window.sessionController.hostedCampaignOpen
                    text: qsTr("Codice %1").arg(window.sessionController.inviteCode)
                    font.weight: Font.DemiBold
                    Accessible.name: qsTr("Codice d’invito %1").arg(window.sessionController.inviteCode)
                }

                Button {
                    visible: window.sessionController.hostedCampaignOpen
                    implicitWidth: Math.max(60, implicitContentWidth + leftPadding + rightPadding)
                    text: qsTr("Copia")
                    Accessible.name: qsTr("Copia codice d’invito")
                    onClicked: window.sessionController.copyInvitationText(window.sessionController.inviteCode)
                }

                Button {
                    visible: window.inSession
                    enabled: !window.sessionController.busy &&
                             (!window.sessionController.hostedCampaignOpen || window.hosting ||
                              (window.hostedPage !== null && window.hostedPage.portValid))
                    text: {
                        if (!window.sessionController.hostedCampaignOpen)
                            return qsTr("Disconnetti")
                        if (window.hosting)
                            return qsTr("Ferma hosting")
                        return window.sessionController.busy ? qsTr("Avvio in corso…") : qsTr("Avvia hosting")
                    }
                    onClicked: {
                        if (window.hosting)
                            window.sessionController.stopHosting()
                        else if (window.hostedPage !== null)
                            window.hostedPage.startHosting()
                        else
                            window.sessionController.disconnectSession()
                    }
                }
            }
        }
    }

    StackView {
        id: navigation

        anchors.fill: parent
        initialItem: ScrollView {
            id: home

            padding: 20
            contentWidth: availableWidth

            ColumnLayout {
                width: Math.min(320, home.availableWidth)
                spacing: 12

                HomeTile {
                    text: qsTr("Sessioni")
                    onClicked: navigation.push(sessionsPage, StackView.Immediate)
                }

                HomeTile {
                    text: qsTr("Schede")
                    onClicked: navigation.push(savedSheetsPage, StackView.Immediate)
                }
            }
        }
    }

    Component {
        id: savedSheetsPage
        SavedSheetsPage { controller: window.sheetController }
    }

    Component {
        id: sheetPage
        SheetView {
            id: localSheet
            identity: window.sheetController.identity
            combatEnabled: true
            abilityScoresData: window.sheetController.abilityScores
            skillScoresData: window.sheetController.skillScores
            savingThrowsData: window.sheetController.savingThrows
            armorClassData: window.sheetController.armorClass
            hitPointsData: window.sheetController.hitPoints
            movementData: window.sheetController.movement
            defensesData: window.sheetController.defenses
            initiativeData: window.sheetController.initiative
            activeConditionsData: window.sheetController.activeConditions
            actionsData: window.sheetController.actions
            attacksData: window.sheetController.attacks
            baseAttackBonusData: window.sheetController.baseAttackBonus
            combatManeuversData: window.sheetController.combatManeuvers
            attackRoutinesData: window.sheetController.attackRoutines
            enhancementsTabPage: Component {
                Column {
                    spacing: 8
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    EnhancementsPanel {
                        width: parent.width
                        enhancements: window.sheetController.enhancements
                        expressionTargets: enhancements.expressionTargets ?? []
                        effectResources: (enhancements.modifierResources ?? []).map(id => ({id, name: id,
                            kinds: [EnhancementsPanel.Bonus, EnhancementsPanel.Penalty]}))
                            .concat((enhancements.contributionResources ?? []).map(id => ({id, name: id,
                                kinds: [EnhancementsPanel.Contribution]})))
                            .sort((first, second) => first.id.localeCompare(second.id))
                        bonusTypes: window.sheetController.enhancementBonusTypes()
                        onAddEnhancementGroupRequested: definition => window.sheetController.addEnhancementGroup(definition)
                        onAddEnhancementRequested: (groupId, definition) => window.sheetController.addEnhancement(groupId, definition)
                        onRemoveEnhancementRequested: (groupId, effectId) => window.sheetController.removeEnhancement(groupId, effectId)
                        onRemoveEnhancementGroupRequested: groupId => window.sheetController.removeEnhancementGroup(groupId)
                        onSetEnhancementGroupActiveRequested: (groupId, active) => window.sheetController.setEnhancementGroupActive(groupId, active)
                    }
                }
            }
            conditionsTabPage: Component {
                Column {
                    spacing: 8
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    ConditionsPanel {
                        width: parent.width
                        conditions: window.sheetController.conditions
                        automaticConditionIds: conditions.automaticConditionIds ?? []
                        onAddConditionRequested: definition => window.sheetController.addCondition(definition)
                        onRemoveConditionsRequested: ids => window.sheetController.removeConditions(ids)
                    }
                }
            }
            spellsTabPage: Component {
                Column {
                    spacing: 24
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    SpellcastingsPanel {
                        width: parent.width
                        spellcastings: window.sheetController.spellcastings
                        spellNames: window.sheetController.spellCatalogData.names ?? ({})
                        spellCatalog: window.sheetController.spellCatalogData.catalog ?? []
                        spellLevelsByClass: window.sheetController.spellCatalogData.levels ?? ({})
                        metamagicDetails: [
                            {name: qsTr("Incantesimi Potenziati"), levelAdjustment: 2},
                            {name: qsTr("Incantesimi Ingranditi"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi Estesi"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi Intensificati")},
                            {name: qsTr("Incantesimi Massimizzati"), levelAdjustment: 3},
                            {name: qsTr("Incantesimi Rapidi"), levelAdjustment: 4},
                            {name: qsTr("Incantesimi Silenziosi"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi Immobili"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi Ampliati"), levelAdjustment: 3},
                            {name: qsTr("Incantesimi Elementali"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi a Distanza")},
                            {name: qsTr("Incantesimi Intensificati (dadi)"), levelAdjustment: 1},
                            {name: qsTr("Incantesimi Selettivi"), levelAdjustment: 1}
                        ]
                        canPrepareSpell: (casting, slot, spellId, metamagic) =>
                            window.sheetController.spellPreparationCount(casting.id, slot, spellId, metamagic ?? []) > 0
                        preparationSlotCount: (casting, slot, spellId, metamagic) =>
                            window.sheetController.spellPreparationCount(casting.id, slot, spellId, metamagic ?? [])
                        preparationMetamagicOptions: (casting, slot, spellId) =>
                            window.sheetController.spellMetamagicOptions(SheetController.Prepare, casting, slot, {spellId})
                        canCastSpontaneousSpell: (casting, slot, spellId) =>
                            window.sheetController.canConsumeSpell(SheetController.ConsumeSpontaneous, casting.id, slot, {spellId})
                        castingMetamagicOptions: (casting, entry, replacement) =>
                            window.sheetController.spellMetamagicOptions(casting.mode === SpellcastingsPanel.Prepared
                                ? SheetController.ConsumePrepared : SheetController.ConsumeSpontaneous, casting, entry.slot,
                                {spellId: entry.spellId, replacement: replacement ?? null})
                        conversionSpellOptions: (casting, entry) => window.sheetController.spellConversionOptions(casting.id, entry.slot)
                        canPrepareFlexibleSpell: (casting, slot, spellId) =>
                            window.sheetController.spellPreparationCount(casting.id, slot, spellId, []) > 0
                        flexiblePreparationMetamagicOptions: (casting, slot, spellId) =>
                            window.sheetController.spellMetamagicOptions(SheetController.PrepareFlexible, casting, slot, {spellId})
                        canCastFlexibleSpell: (casting, slot, preparation) =>
                            window.sheetController.canConsumeSpell(SheetController.ConsumeFlexible, casting.id, slot, {preparation: preparation.preparationId})
                        flexibleCastingMetamagicOptions: (casting, slot, preparation) =>
                            window.sheetController.spellMetamagicOptions(SheetController.ConsumeFlexible, casting, slot,
                                {preparation: preparation.preparationId})
                        onPrepareSpellRequested: (id, slots, spellId, metamagic) =>
                            window.sheetController.spellCommand(SheetController.Prepare, id, {slots, spellId, metamagic})
                        onConsumeSpellRequested: (id, slot, metamagic, replacement) =>
                            window.sheetController.spellCommand(SheetController.ConsumePrepared, id, {slot, metamagic, replacement})
                        onCastSpontaneousSpellRequested: (id, slot, spellId, metamagic) =>
                            window.sheetController.spellCommand(SheetController.ConsumeSpontaneous, id, {slot, spellId, metamagic})
                        onPrepareFlexibleSpellRequested: (id, slot, spellId, metamagic) =>
                            window.sheetController.spellCommand(SheetController.PrepareFlexible, id, {slot, spellId, metamagic})
                        onCastFlexibleSpellRequested: (id, slot, preparation, metamagic, mixingPermissionId) =>
                            window.sheetController.spellCommand(SheetController.ConsumeFlexible, id, {slot, preparation, metamagic, mixingPermissionId})
                        onClearFlexiblePreparationRequested: (id, slot) =>
                            window.sheetController.spellCommand(SheetController.ClearPreparation, id, {slot})
                        onClearSpellSlotRequested: (id, slot) =>
                            window.sheetController.spellCommand(SheetController.ClearSlot, id, {slot})
                        onResetSpellSlotsRequested: id => window.sheetController.spellCommand(SheetController.ResetSlots, id, {})
                        onAddSpellToRepertoireRequested: (id, spellId) =>
                            window.sheetController.spellCommand(SheetController.AddToRepertoire, id, {spellId})
                        onSelectRepertoireSpellRequested: (id, selection) =>
                            window.sheetController.spellCommand(SheetController.SelectRepertoire, id, selection)
                    }
                    GrantedSpellsPanel {
                        width: parent.width
                        grantedSpells: window.sheetController.grantedSpells.granted ?? []
                        spellNames: window.sheetController.spellCatalogData.names ?? ({})
                        onCastGrantedSpellRequested: id => window.sheetController.spellCommand(SheetController.ConsumeGranted, id, {})
                    }
                    Label {
                        width: parent.width
                        visible: Object.keys(window.sheetController.lastCast.lastCast ?? {}).length > 0
                        text: qsTr("Incantesimo lanciato.")
                    }
                }
            }
            featsTabPage: Component {
                Column {
                    spacing: 8
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    TalentsTab {
                        width: parent.width
                        reminders: window.sheetController.reminders.reminders
                        tabForChoice: type => localSheet.tabForChoice(type)
                        onOpenChoiceRequested: type => localSheet.openChoiceTab(type)
                        languagesPage: Component {
                            LanguagesPanel {
                                languages: window.sheetController.languages.languages
                                languageNames: ({common: qsTr("Comune"), elven: qsTr("Elfico"), draconic: qsTr("Draconico"),
                                    dwarven: qsTr("Nanico"), orc: qsTr("Orchesco"), sylvan: qsTr("Silvano"),
                                    halfling: qsTr("Halfling"), gnome: qsTr("Gnomesco"), goblin: qsTr("Goblin"),
                                    giant: qsTr("Gigante"), celestial: qsTr("Celestiale"), infernal: qsTr("Infernale"),
                                    abyssal: qsTr("Abissale"), undercommon: qsTr("Sottocomune")})
                                extraLanguageIds: (languages?.options ?? []).reduce((ids, entry) => ids.concat(entry.languageIds), [])
                                onAddRequested: (id, origin) => window.sheetController.addLanguage(id, origin)
                                onGrantRequested: id => window.sheetController.addLanguage(id, LanguagesPanel.Extra)
                                onCustomGrantRequested: name => window.sheetController.addLanguage(name, LanguagesPanel.Extra)
                                onCustomLearnRequested: name => window.sheetController.addLanguage(name, LanguagesPanel.CustomLinguistics)
                                onRemoveRequested: id => window.sheetController.removeLanguage(id)
                            }
                        }
                        featsPage: Component {
                            FeatsPanel {
                                id: liveFeats
                                feats: window.sheetController.feats.feats

                                function openCatalog(slotId) {
                                    picker.slotId = slotId;
                                    picker.extra = !slotId;
                                    picker.acquisitionLabel = slotId
                                        ? (feats?.slots ?? []).find(slot => slot.id === slotId)?.source ?? ""
                                        : qsTr("Talento extra · senza slot né prerequisiti");
                                    picker.catalog = window.sheetController.featCatalog(slotId);
                                    picker.open();
                                }
                                function chooseFeat(slotId, featId) {
                                    const feat = window.sheetController.featDetail(featId);
                                    if (!feat.id) return;
                                    if (!feat.choices.length) {
                                        window.sheetController.selectFeat(slotId, featId, {});
                                        return;
                                    }
                                    choiceDialog.slotId = slotId;
                                    choiceDialog.feat = feat;
                                    choiceDialog.draft = ({});
                                    choiceDialog.open();
                                }

                                onSelectRequested: id => openCatalog(id)
                                onExtraRequested: openCatalog("")
                                onRemoveRequested: id => window.sheetController.removeFeat(id, false)
                                onRemoveExtraSlotRequested: id => window.sheetController.removeFeat(id, true)

                                FeatCatalogPanel {
                                    id: picker
                                    catalog: []
                                    onAddRequested: (slotId, featId) => liveFeats.chooseFeat(slotId, featId)
                                    onExtraRequested: featId => liveFeats.chooseFeat("", featId)
                                }
                                Dialog {
                                    id: choiceDialog
                                    parent: Overlay.overlay
                                    anchors.centerIn: parent
                                    modal: true
                                    title: feat?.name ?? qsTr("Scelte del talento")
                                    width: Math.min(520, (parent?.width ?? window.width) - 24)
                                    property string slotId: ""
                                    property var feat: null
                                    property var draft: ({})
                                    readonly property bool complete: !!feat && feat.choices.every(choice =>
                                        (draft[choice.id]?.length ?? 0) === choice.selectionCount)
                                    function toggle(choiceId, optionId) {
                                        const current = draft[choiceId] ?? [];
                                        const selected = current.includes(optionId)
                                            ? current.filter(id => id !== optionId) : current.concat(optionId);
                                        draft = Object.assign({}, draft, {[choiceId]: selected});
                                    }
                                    footer: DialogButtonBox {
                                        Button { text: qsTr("Annulla"); onClicked: choiceDialog.reject() }
                                        Button {
                                            text: qsTr("Conferma")
                                            enabled: choiceDialog.complete
                                            onClicked: choiceDialog.accept()
                                        }
                                    }
                                    onAccepted: window.sheetController.selectFeat(slotId, feat.id, draft)
                                    contentItem: ScrollView {
                                        id: featChoicesScroll
                                        contentWidth: availableWidth
                                        implicitHeight: Math.min(410, choiceGroups.implicitHeight)
                                        clip: true
                                        Column {
                                            id: choiceGroups
                                            width: featChoicesScroll.availableWidth
                                            spacing: 12
                                            Repeater {
                                                model: choiceDialog.feat?.choices ?? []
                                                delegate: Column {
                                                    id: choiceGroup
                                                    required property var modelData
                                                    width: choiceGroups.width
                                                    readonly property var selected: choiceDialog.draft[modelData.id] ?? []
                                                    Label {
                                                        width: parent.width
                                                        text: qsTr("%1 (%2/%3)").arg(choiceGroup.modelData.prompt)
                                                            .arg(choiceGroup.selected.length).arg(choiceGroup.modelData.selectionCount)
                                                        font.bold: true
                                                        textFormat: Text.PlainText
                                                        wrapMode: Text.Wrap
                                                    }
                                                    Repeater {
                                                        model: choiceGroup.modelData.options
                                                        delegate: CheckBox {
                                                            required property var modelData
                                                            text: modelData.name
                                                            checked: choiceGroup.selected.includes(modelData.id)
                                                            enabled: checked || choiceGroup.selected.length < choiceGroup.modelData.selectionCount
                                                            onClicked: choiceDialog.toggle(choiceGroup.modelData.id, modelData.id)
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
                }
            }
            classesTabPage: Component {
                Column {
                    id: classesPage
                    readonly property var classCatalog: window.sheetController.classCatalog()
                    spacing: 8
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    ClassesPanel {
                        width: parent.width
                        classes: window.sheetController.classes
                        catalog: classesPage.classCatalog
                        loadArchetypes: type => window.sheetController.archetypeCatalog(type)
                        loadArchetypeDescription: id => window.sheetController.archetypeDescription(id)
                        onAddLevelRequested: type => window.sheetController.addClassLevel(type)
                        onRemoveLevelRequested: type => window.sheetController.removeClassLevel(type)
                        onFavoredClassRequested: (type, favored) => window.sheetController.setFavoredClass(type, favored)
                        onHitDieResultRequested: (type, level, result) => window.sheetController.setClassHitDieResult(type, level, result)
                        onFavoredBonusRequested: (type, level, optionId) => window.sheetController.selectFavoredClassBonus(type, level, optionId)
                        onAddArchetypeRequested: (type, id) => window.sheetController.setClassArchetype(type, id, true)
                        onRemoveArchetypeRequested: (type, id) => window.sheetController.setClassArchetype(type, id, false)
                        onSelectOptionRequested: (type, acquisitionId, choiceId, optionId) =>
                            window.sheetController.setClassOption(type, acquisitionId, choiceId, optionId, true)
                        onUnselectOptionRequested: (type, acquisitionId, choiceId, optionId) =>
                            window.sheetController.setClassOption(type, acquisitionId, choiceId, optionId, false)
                    }
                }
            }
            raceTabPage: Component {
                Column {
                    spacing: 8
                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }
                    RacePanel {
                        width: parent.width
                        race: window.sheetController.race
                        catalog: window.sheetController.raceCatalog()
                        alternateFeatures: race.alternateFeatures ?? []
                        onSetRaceRequested: id => window.sheetController.setRace(id)
                        onSelectAlternateTraitRequested: id => window.sheetController.selectAlternateRacialTrait(id)
                        onRemoveAlternateTraitRequested: id => window.sheetController.removeAlternateRacialTrait(id)
                        onChangeChoiceRequested: (elementId, choiceId, optionIds) =>
                            window.sheetController.setRacialChoice(elementId, choiceId, optionIds)
                    }
                }
            }
            inventoryTabPage: Component {
                Column {
                    spacing: 8

                    Label {
                        width: parent.width
                        visible: text.length > 0
                        text: window.sheetController.errorMessage
                        color: "#a13c2a"
                        wrapMode: Text.Wrap
                    }

                    InventoryPanel {
                        width: parent.width
                        inventory: Object.assign({}, window.sheetController.inventoryItems,
                            window.sheetController.inventoryContainers, window.sheetController.inventoryEquipment)
                        encumbrance: window.sheetController.encumbrance.encumbrance
                        encumbranceBelowItems: true
                        equipmentFooter: Component {
                            MoneyPanel {
                                money: window.sheetController.inventoryMoney.money
                                containers: window.sheetController.inventoryContainers.containers ?? []
                                onAddMoneyRequested: (denomination, quantity, containerId) =>
                                    window.sheetController.changeMoney(denomination, quantity, containerId)
                                onConvertMoneyRequested: (from, to, cost, proceeds, sourceId, destinationId) =>
                                    window.sheetController.convertMoney(from, to, cost, proceeds, sourceId, destinationId)
                            }
                        }
                        onAddContainerRequested: definition => window.sheetController.addContainer(definition)
                        onAddCatalogItemRequested: {
                            catalogPicker.entries = window.sheetController.itemCatalog()
                            catalogPicker.open()
                        }
                        onAddCustomItemRequested: definition => window.sheetController.addCustomItem(definition)
                        onMoveItemRequested: (id, containerId) => window.sheetController.moveItem(id, containerId)
                        onItemQuantityChangeRequested: (id, quantity) => window.sheetController.setItemQuantity(id, quantity)
                        onMergeCoinStacksRequested: (sourceId, targetId) => window.sheetController.mergeCoinStacks(sourceId, targetId)
                        onSplitCoinStackRequested: (id, quantity) => window.sheetController.splitCoinStack(id, quantity)
                        onRestoreItemUsagePoolRequested: (id, poolId, amount) => window.sheetController.restoreItemUsagePool(id, poolId, amount)
                        onRefillItemUsagePoolRequested: (id, poolId) => window.sheetController.refillItemUsagePool(id, poolId)
                        onReorderItemRequested: (id, beforeId) => window.sheetController.reorderItem(id, beforeId)
                        onMoveAndReorderItemRequested: (id, containerId, beforeId) =>
                            window.sheetController.moveAndReorderItem(id, containerId, beforeId)
                        onEquipItemRequested: (id, replacedId) => window.sheetController.equipItem(id, replacedId)
                        onUnequipItemRequested: (id, containerId) => window.sheetController.unequipItem(id, containerId)
                    }

                    Dialog {
                        id: catalogPicker
                        parent: Overlay.overlay
                        anchors.centerIn: parent
                        title: qsTr("Catalogo degli oggetti")
                        modal: true
                        width: Math.min(450, (parent?.width ?? window.width) - 24)
                        height: Math.min(480, (parent?.height ?? window.height) - 24)
                        standardButtons: Dialog.Close
                        property var entries: []
                        onOpened: {
                            itemSearch.clear()
                            itemSearch.forceActiveFocus()
                        }

                        contentItem: ColumnLayout {
                            TextField {
                                id: itemSearch
                                Layout.fillWidth: true
                                placeholderText: qsTr("Cerca un oggetto")
                            }
                            Label {
                                visible: catalogResults.count === 0
                                text: qsTr("Nessun oggetto trovato.")
                            }
                            ListView {
                                id: catalogResults
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: catalogPicker.entries.filter(entry => entry.name.toLocaleLowerCase()
                                    .includes(itemSearch.text.trim().toLocaleLowerCase()))
                                delegate: ItemDelegate {
                                    required property var modelData
                                    width: catalogResults.width
                                    text: modelData.name
                                    onClicked: {
                                        window.sheetController.addCatalogItem(modelData.id)
                                        catalogPicker.close()
                                    }
                                }
                                ScrollBar.vertical: ScrollBar {}
                            }
                        }
                    }
                }
            }
            identityErrorMessage: window.sheetController.errorMessage
            notes: window.sheetController.notes
            onIdentityFieldChangeRequested: (field, value) => window.sheetController.setIdentityField(field, value)
            onNotesChangeRequested: notes => window.sheetController.setNotes(notes)
            hitPointEditingEnabled: true
            statisticsEditingEnabled: true
            onAbilityBaseChangeRequested: (type, value) => window.sheetController.setAbilityBaseValue(type, value)
            onSkillRanksChangeRequested: (resourceName, ranks) => window.sheetController.setSkillRanks(resourceName, ranks)
            onSkillSpecializationAddRequested: (type, name) => window.sheetController.addSkillSpecialization(type, name)
            onSkillSpecializationRemoveRequested: (type, specializationId) => window.sheetController.removeSkillSpecialization(type, specializationId)
            onClassSkillChangeRequested: (type, enabled) => window.sheetController.setCustomClassSkill(type, enabled)
            onTemporaryAddRequested: (source, amount) => window.sheetController.addTemporaryHitPoints(source, amount)
            onTemporaryRemoveRequested: source => window.sheetController.removeTemporaryHitPoints(source)
            onDamageRequested: (amount, nonLethal) => window.sheetController.damage(amount, nonLethal)
            onHealRequested: amount => window.sheetController.heal(amount)
            onAttackRoutineAddRequested: routineId => window.sheetController.addAttackRoutine(routineId)
            onAttackRoutineRemoveRequested: attackId => window.sheetController.removeAttackRoutine(attackId)
            onAttackRoutineRenameRequested: (attackId, name) => window.sheetController.renameAttackRoutine(attackId, name)
            onAttackRoutineReorderRequested: (attackId, beforeAttackId) => window.sheetController.reorderAttackRoutine(attackId, beforeAttackId)
            onAttackStrikeAssignRequested: (attackId, slotId, strikeGrantId) => window.sheetController.assignAttackStrike(attackId, slotId, strikeGrantId)
        }
    }

    Connections {
        target: window.sheetController
        function onSheetOpened() {
            if (navigation.currentItem instanceof SavedSheetsPage)
                navigation.push(sheetPage, StackView.Immediate)
        }
    }

    Component {
        id: sessionsPage

        CampaignsPage {
            controller: window.sessionController
            localName: displayNameField.text
        }
    }

    Settings {
        category: "Profile"
        property alias displayName: displayNameField.text
    }

    ParticipantPermissionsDialog {
        id: permissionsDialog
        controller: window.sessionController
        canEditDelegation: window.hosting
    }

    Connections {
        target: window.sessionController

        function onSessionOpened() {
            const page = window.sessionController.hostedCampaignOpen ? hostedSessionPage : activeSessionPage
            navigation.push(page, StackView.Immediate)
        }

        function onSessionClosed() {
            permissionsDialog.close()
            navigation.pop(navigation.get(navigation.depth - 2), StackView.Immediate)
        }
    }

    Component {
        id: hostedSessionPage

        HostedCampaignPage {
            controller: window.sessionController
            localName: displayNameField.text
            onPermissionsRequested: (userId, displayName) => permissionsDialog.openFor(userId, displayName)
        }
    }

    Component {
        id: activeSessionPage

        ScrollView {
            id: joinedPage

            padding: 20
            contentWidth: availableWidth

            GridLayout {
                width: joinedPage.availableWidth
                columns: width < 540 ? 1 : 2
                columnSpacing: 24
                rowSpacing: 16

                Item {
                    Layout.fillWidth: true
                    implicitHeight: clientError.implicitHeight
                    visible: joinedPage.availableWidth >= 540 || clientError.visible

                    Label {
                        id: clientError
                        width: parent.width
                        visible: text.length > 0
                        text: window.sessionController.errorMessage
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }
                }

                ParticipantsPanel {
                    Layout.preferredWidth: 260
                    Layout.fillWidth: joinedPage.availableWidth < 540
                    Layout.alignment: Qt.AlignTop
                    participants: window.sessionController.participants
                    onPermissionsRequested: (userId, displayName) => permissionsDialog.openFor(userId, displayName)
                }
            }
        }
    }

    component HomeTile: Button {
        id: tile

        Layout.fillWidth: true
        Layout.preferredHeight: 112
        font.pixelSize: 22
        font.weight: Font.DemiBold
        palette.button: "#ffffff"

        background: Rectangle {
            radius: 8
            color: tile.down ? "#e7edf4" : tile.palette.button
            border.color: tile.visualFocus || tile.hovered ? tile.palette.highlight : "#e1e5ea"
            border.width: tile.visualFocus ? 2 : 1
        }
    }
}
