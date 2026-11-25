pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    enum ClassType { Barbarian, Bard, Cleric, Druid, Fighter, Rogue, Wizard, Monk, Paladin, Ranger, Sorcerer, Alchemist, Cavalier, Summoner, Witch, Inquisitor, Magus, Shifter, Oracle, Gunslinger, Vigilante, Antipaladin, Ninja, Samurai, Arcanist, Brawler, Hunter, Swashbuckler, Investigator, Bloodrager, Slayer, Warpriest, Skald, Shaman, Kineticist, Medium, Mesmerist, Occultist, Psychic, Spiritualist, UnchainedBarbarian, UnchainedSummoner, UnchainedRogue, UnchainedMonk, Adept }
    required property var classes
    property var catalog: []
    property var loadArchetypes: type => []
    property var loadArchetypeDescription: id => ""
    property var archetypeCatalog: []
    signal addArchetypeRequested(int classType, string archetypeId)
    signal removeArchetypeRequested(int classType, string archetypeId)
    property var editingArchetypeClass: null
    readonly property alias archetypePopup: archetypePicker
    function archetypeSelected(id) { return (editingArchetypeClass?.archetypes ?? []).some(entry => entry.id === id); }
    function archetypeCandidates() {
        return archetypeCatalog.filter(entry => !archetypeSearch.text || entry.name.toLocaleLowerCase().includes(archetypeSearch.text.toLocaleLowerCase()))
            .slice().sort((a, b) => a.name.localeCompare(b.name, "it"));
    }
    Dialog {
        id: archetypePicker
        parent: Overlay.overlay
        modal: true
        title: qsTr("Archetipi · %1").arg(root.editingArchetypeClass?.name ?? "")
        width: Math.min(520, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        onAboutToShow: {
            archetypeSearch.clear()
            root.archetypeCatalog = root.loadArchetypes(root.editingArchetypeClass.type)
        }
        footer: DialogButtonBox { Button { text: qsTr("Annulla"); onClicked: archetypePicker.reject() } }
        contentItem: Column {
            spacing: 10
            TextField { id: archetypeSearch; objectName: "archetypeSearch"; width: parent.width; placeholderText: qsTr("Cerca archetipo…") }
            ListView {
                id: archetypeList
                width: parent.width
                height: Math.min(320, Math.max(48, count * 44), (archetypePicker.parent?.height ?? 600) * 0.45)
                model: root.archetypeCandidates()
                clip: true
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: archetypeCandidate
                    required property var modelData
                    objectName: "archetypeCandidate_" + modelData.id
                    width: archetypeList.width
                    height: 44
                    enabled: !root.archetypeSelected(modelData.id)
                    contentItem: Label { text: archetypeCandidate.modelData.name + (root.archetypeSelected(archetypeCandidate.modelData.id) ? qsTr(" · Selezionato") : ""); textFormat: Text.PlainText; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
                    ToolTip.visible: hovered
                    ToolTip.text: hovered ? root.loadArchetypeDescription(modelData.id) : ""
                    onClicked: { root.addArchetypeRequested(root.editingArchetypeClass.type, modelData.id); archetypePicker.close(); }
                }
            }
            Label { visible: !archetypeList.count; text: qsTr("Nessun archetipo disponibile."); color: "#687c88" }
        }
    }
    signal addLevelRequested(int classType)
    signal removeLevelRequested(int classType)
    signal favoredClassRequested(int classType, bool favored)
    signal hitDieResultRequested(int classType, int classLevel, int result)
    signal favoredBonusRequested(int classType, int classLevel, string optionId)
    enum BenefitType { HitPoint, SkillRank, Other }
    property var editingBonusClass: null
    property int editingBonusLevel: 0
    readonly property alias bonusPopup: bonusPicker
    function favoredEntry(type) { return (classes?.favoredClasses?.classes ?? []).find(entry => entry.type === type) ?? null; }
    function selectedBonus(type, level) {
        const entry = favoredEntry(type);
        const selection = (entry?.selections ?? []).find(choice => choice.classLevel === level && choice.active);
        return (entry?.options ?? []).find(option => option.id === selection?.optionId) ?? null;
    }
    function bonusSymbols(option) {
        return option ? (option.benefitTypes?.length ? option.benefitTypes : [ClassesPanel.Other])
            .map(type => type === ClassesPanel.HitPoint ? "♥" : type === ClassesPanel.SkillRank ? "📖" : "✦").join(" ") : "+";
    }
    function bonusTypes(option) { return option ? (option.benefitTypes?.length ? option.benefitTypes : [ClassesPanel.Other]) : []; }
    function editBonus(type, level) {
        editingBonusClass = favoredEntry(type);
        editingBonusLevel = level;
        bonusPicker.open();
    }
    Dialog {
        id: bonusPicker
        parent: Overlay.overlay
        modal: true
        title: qsTr("Bonus di classe preferita · livello %1").arg(root.editingBonusLevel)
        width: Math.min(520, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        footer: DialogButtonBox { Button { text: qsTr("Annulla"); onClicked: bonusPicker.reject() } }
        contentItem: ListView {
            id: bonusList
            implicitHeight: Math.min(320, Math.max(64, contentHeight), (bonusPicker.parent?.height ?? 600) * 0.45)
            model: root.editingBonusClass?.options ?? []
            clip: true
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: bonusOption
                required property var modelData
                width: bonusList.width
                objectName: "favoredBonusOption_" + modelData.id
                enabled: modelData.available === true
                contentItem: Column {
                    spacing: 4
                    RowLayout {
                        width: parent.width
                        FavoredBonusIcons { types: root.bonusTypes(bonusOption.modelData) }
                        Label { Layout.fillWidth: true; text: bonusOption.modelData.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.bold: true }
                    }
                    Label { width: parent.width; text: bonusOption.modelData.description; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: "#687c88" }
                    Label { visible: bonusOption.modelData.selectionsPerBenefit > 1; text: qsTr("Un beneficio ogni %1 scelte · %2 scelte totali").arg(bonusOption.modelData.selectionsPerBenefit).arg(bonusOption.modelData.selections); wrapMode: Text.Wrap; width: parent.width }
                }
                onClicked: { root.favoredBonusRequested(root.editingBonusClass.type, root.editingBonusLevel, modelData.id); bonusPicker.close(); }
            }
        }
    }
    property var editingHitDie: null
    readonly property alias hitDiePopup: hitDiePicker
    function isFavored(type) { return (classes?.favoredClasses?.selectedClasses ?? []).includes(type); }
    function editHitDie(entry, level) {
        editingHitDie = {type: entry.type, level: level, sides: entry.hitDie};
        hitDieInput.text = entry.hitDieResults?.[level]?.toString() ?? "";
        hitDiePicker.open();
        hitDieInput.forceActiveFocus();
        hitDieInput.selectAll();
    }
    Dialog {
        id: hitDiePicker
        parent: Overlay.overlay
        modal: true
        title: qsTr("PF del livello %1 · d%2").arg(root.editingHitDie?.level ?? "").arg(root.editingHitDie?.sides ?? "")
        width: Math.min(340, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        function submit() {
            if (!hitDieInput.acceptableInput) return;
            root.hitDieResultRequested(root.editingHitDie.type, root.editingHitDie.level, Number(hitDieInput.text));
            close();
        }
        contentItem: TextField {
            id: hitDieInput
            objectName: "hitDieInput"
            placeholderText: qsTr("Risultato del dado")
            validator: IntValidator { bottom: 1; top: root.editingHitDie?.sides ?? 0 }
            onAccepted: hitDiePicker.submit()
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: hitDiePicker.reject() }
            Button { objectName: "confirmHitDie"; text: qsTr("Conferma"); enabled: hitDieInput.acceptableInput; onClicked: hitDiePicker.submit() }
        }
    }
    readonly property alias classPopup: classPicker
    signal selectOptionRequested(int classType, string acquisitionId, string choiceId, string optionId)
    signal unselectOptionRequested(int classType, string acquisitionId, string choiceId, string optionId)
    property var editingChoice: null
    property int editingClassType: -1
    property string editingAcquisitionId: ""
    readonly property alias choicePopup: choicePicker
    onClassesChanged: { if (choicePicker) choicePicker.close(); if (classPicker) classPicker.close(); if (hitDiePicker) hitDiePicker.close(); if (bonusPicker) bonusPicker.close(); if (archetypePicker) archetypePicker.close(); }
    onVisibleChanged: if (!visible) { choicePicker.close(); classPicker.close(); hitDiePicker.close(); bonusPicker.close(); archetypePicker.close(); }

    function availableClasses() {
        return catalog.filter(entry => !(classes?.classes ?? []).some(current => current.type === entry.type))
            .filter(entry => !classSearch.text || entry.name.toLocaleLowerCase().includes(classSearch.text.toLocaleLowerCase()))
            .slice().sort((a, b) => a.name.localeCompare(b.name, "it"));
    }
    Dialog {
        id: classPicker
        property bool choosingFavored: false
        parent: Overlay.overlay
        modal: true
        title: choosingFavored ? qsTr("Classi preferite") : qsTr("Aggiungi classe")
        width: Math.min(480, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        onAboutToShow: classSearch.clear()
        footer: DialogButtonBox { Button { text: qsTr("Annulla"); onClicked: classPicker.reject() } }
        contentItem: Column {
            spacing: 10
            TextField { id: classSearch; objectName: "classSearch"; width: parent.width; placeholderText: qsTr("Cerca classe…"); selectByMouse: true }
            ListView {
                id: classList
                width: parent.width
                height: Math.min(320, Math.max(48, count * 44), (classPicker.parent?.height ?? 600) * 0.45)
                model: classPicker.choosingFavored ? (root.catalog ?? [])
                    .filter(entry => entry.name.toLocaleLowerCase().includes(classSearch.text.trim().toLocaleLowerCase()))
                    .sort((a, b) => a.name.localeCompare(b.name, "it")) : root.availableClasses()
                clip: true
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: candidate
                    required property var modelData
                    objectName: "newClass_" + modelData.type
                    width: classList.width
                    height: 44
                    contentItem: Label {
                        text: (classPicker.choosingFavored ? (root.isFavored(candidate.modelData.type) ? "★ " : "☆ ") : "") + candidate.modelData.name
                        textFormat: Text.PlainText
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (classPicker.choosingFavored)
                            root.favoredClassRequested(modelData.type, !root.isFavored(modelData.type));
                        else
                            root.addLevelRequested(modelData.type);
                        classPicker.close();
                    }
                }
            }
            Label { visible: !classList.count; text: qsTr("Nessuna classe disponibile."); color: "#687c88" }
        }
    }

    function editChoice(classType, acquisitionId, choice) {
        editingClassType = classType;
        editingAcquisitionId = acquisitionId;
        editingChoice = choice;
        choicePicker.open();
    }
    Dialog {
        id: choicePicker
        parent: Overlay.overlay
        modal: true
        title: qsTr("Opzioni del privilegio")
        width: Math.min(540, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        standardButtons: Dialog.Close
        contentItem: Column {
            spacing: 10
            Label { width: parent.width; text: root.editingChoice?.prompt ?? ""; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.bold: true }
            Label { text: qsTr("Da scegliere ancora: %1").arg(root.editingChoice?.remainingSelections ?? 0); color: "#687c88" }
            ListView {
                id: optionList
                width: parent.width
                height: Math.min(320, Math.max(80, contentHeight), (choicePicker.parent?.height ?? 600) * 0.45)
                clip: true
                model: root.editingChoice?.options ?? []
                ScrollBar.vertical: ScrollBar {}
                delegate: RowLayout {
                    id: optionRow
                    required property var modelData
                    width: optionList.width
                    spacing: 10
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { Layout.fillWidth: true; text: optionRow.modelData.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.bold: true }
                        Label { Layout.fillWidth: true; visible: !!text; text: optionRow.modelData.description ?? ""; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: "#687c88" }
                        Label { visible: optionRow.modelData.selected === true; text: qsTr("Selezionata"); color: "#365f75" }
                    }
                    Button {
                        objectName: "classChoiceOption_" + optionRow.modelData.id
                        text: optionRow.modelData.selected ? "×" : "+"
                        enabled: optionRow.modelData.selected === true || root.editingChoice?.remainingSelections > 0
                        Accessible.name: optionRow.modelData.selected ? qsTr("Rimuovi %1").arg(optionRow.modelData.name) : qsTr("Scegli %1").arg(optionRow.modelData.name)
                        onClicked: {
                            if (optionRow.modelData.selected)
                                root.unselectOptionRequested(root.editingClassType, root.editingAcquisitionId, root.editingChoice.id, optionRow.modelData.id);
                            else
                                root.selectOptionRequested(root.editingClassType, root.editingAcquisitionId, root.editingChoice.id, optionRow.modelData.id);
                            choicePicker.close();
                        }
                    }
                }
            }
        }
    }
    implicitWidth: 900
    implicitHeight: content.height

    function activeFeatures(entry) {
        return (entry.features ?? []).map(feature => ({
            id: feature.id, name: feature.name, description: feature.description,
            acquisitions: (feature.acquisitions ?? []).filter(acquisition => acquisition.active === true)
        })).filter(feature => feature.acquisitions.length > 0);
    }
    function missingChoices(entry) {
        return activeFeatures(entry).reduce((all, feature) => all.concat(feature.acquisitions.reduce((choices, acquisition) => choices.concat(
            (acquisition.choices ?? []).filter(choice => choice.remainingSelections > 0)
                .map(choice => ({featureName: feature.name, acquisition: acquisition, choice: choice}))), [])), []);
    }
    function choicesFor(feature) {
        return feature.acquisitions.reduce((all, acquisition) => all.concat((acquisition.choices ?? [])
            .map(choice => ({acquisition: acquisition, choice: choice,
                selected: (choice.options ?? []).filter(option => option.selected === true)}))), [])
            .filter(entry => entry.selected.length > 0);
    }
    function levelText(feature) {
        const levels = [...new Set(feature.acquisitions.map(acquisition => acquisition.level))].sort((a, b) => a - b);
        return levels.length === 1 ? qsTr("Livello %1").arg(levels[0]) : qsTr("Livelli %1").arg(levels.join(" · "));
    }
    Column {
        id: content
        width: root.width
        spacing: 24
        RowLayout {
            width: parent.width
            Label { Layout.fillWidth: true; text: qsTr("Classi"); font.pixelSize: 20; font.bold: true; color: "#253947" }
            Label {
                objectName: "totalClassLevel"
                text: qsTr("Livello totale: %1").arg(root.classes?.level ?? "—")
                font.pixelSize: 15
                color: "#536976"
            }
        }
        Label {
            visible: !(root.classes?.classes?.length)
            width: parent.width
            text: qsTr("Nessuna classe selezionata.")
            wrapMode: Text.Wrap
            color: "#777777"
        }
        Flow {
            width: parent.width
            spacing: 8
            Button {
                objectName: "addClass"
                text: qsTr("+ Aggiungi classe")
                onClicked: { classPicker.choosingFavored = false; classPicker.open(); }
            }
            Button {
                objectName: "chooseFavoredClass"
                text: qsTr("☆ Classi preferite")
                onClicked: { classPicker.choosingFavored = true; classPicker.open(); }
            }
        }
        Repeater {
            model: root.classes?.classes ?? []
            Column {
                id: section
                required property var modelData
                readonly property var features: root.activeFeatures(modelData)
                readonly property var pending: root.missingChoices(modelData)
                objectName: "classSection_" + modelData.type
                width: content.width
                spacing: 16
                Rectangle {
                    width: parent.width
                    implicitHeight: hero.height + 40
                    color: "#e9eef1"
                    border.color: "#c6cfd5"
                    radius: 8
                    Rectangle { x: 0; y: 14; width: 4; height: parent.height - 28; radius: 2; color: "#52768e" }
                    Column {
                        id: hero
                        x: 24; y: 20
                        width: parent.width - 48
                        spacing: 8
                        Label { text: qsTr("CLASSE"); font.pixelSize: 11; font.letterSpacing: 2; font.bold: true; color: "#527080" }
                        RowLayout {
                            width: parent.width
                            ToolButton {
                                id: favoredStar
                                objectName: "favoredClass_" + section.modelData.type
                                Layout.alignment: Qt.AlignBaseline
                                baselineOffset: contentItem.y + contentItem.baselineOffset
                                padding: 0
                                text: root.isFavored(section.modelData.type) ? "★" : "☆"
                                contentItem: Label {
                                    text: favoredStar.text
                                    font.pixelSize: 32
                                    color: root.isFavored(section.modelData.type) ? "#ad7a16" : "#687c88"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Accessible.name: qsTr("Classe preferita")
                                onClicked: root.favoredClassRequested(section.modelData.type, !root.isFavored(section.modelData.type))
                            }
                            Item {
                                Layout.alignment: Qt.AlignBaseline
                                Layout.fillWidth: true
                                implicitHeight: classNames.height
                                baselineOffset: classNameLabel.baselineOffset
                                Flow {
                                    id: classNames
                                    width: parent.width
                                    spacing: 8
                            Label {
                                id: classNameLabel
                                objectName: "className_" + section.modelData.type
                                width: Math.min(implicitWidth, classNames.width)
                                text: section.modelData.name
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 32
                                font.bold: true
                                color: "#20333f"
                            }
                            Label {
                                objectName: "classArchetypeNames_" + section.modelData.type
                                visible: !!section.modelData.archetypes?.length
                                width: Math.min(implicitWidth, classNames.width)
                                text: (section.modelData.archetypes ?? []).map(entry => entry.name).join(", ")
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 17
                                font.italic: true
                                topPadding: 13
                                color: "#536976"
                            }
                                }
                            }
                            Label {
                                objectName: "classLevel_" + section.modelData.type
                                text: qsTr("Livello %1").arg(section.modelData.level)
                                font.pixelSize: 18
                                font.bold: true
                                color: "#365f75"
                            }
                        Flow {
                            Layout.preferredWidth: Math.min(section.modelData.level * 47, hero.width * 0.35)
                            spacing: 5
                            Repeater {
                                model: section.modelData.level
                                Column {
                                    id: dieLevel
                                    required property int index
                                    spacing: 2
                                HitDieButton {
                                    objectName: "hitDie_" + section.modelData.type + "_" + (dieLevel.index + 1)
                                    sides: section.modelData.hitDie ?? 0
                                    result: section.modelData.hitDieResults?.[dieLevel.index + 1] ?? 0
                                    classLevel: dieLevel.index + 1
                                    enabled: sides > 0
                                    onClicked: root.editHitDie(section.modelData, dieLevel.index + 1)
                                }
                                ToolButton {
                                    id: bonusButton
                                    readonly property var option: root.selectedBonus(section.modelData.type, dieLevel.index + 1)
                                    objectName: "favoredBonus_" + section.modelData.type + "_" + (dieLevel.index + 1)
                                    visible: root.favoredEntry(section.modelData.type)?.favored === true
                                    width: 42; height: 30
                                    text: root.bonusSymbols(option)
                                    contentItem: Item {
                                        implicitWidth: bonusIcons.implicitWidth
                                        implicitHeight: 22
                                        FavoredBonusIcons { id: bonusIcons; anchors.centerIn: parent; types: root.bonusTypes(bonusButton.option) }
                                    }
                                    font.pixelSize: 16
                                    background: Rectangle { color: "transparent"; radius: 4; border.color: bonusButton.hovered ? "#52768e" : "#a1adb5"; border.width: bonusButton.option ? 0 : 1 }
                                    ToolTip.visible: hovered
                                    ToolTip.text: option ? option.name + "\n" + option.description : qsTr("Scegli il bonus di classe preferita")
                                    Accessible.name: ToolTip.text
                                    onClicked: root.editBonus(section.modelData.type, dieLevel.index + 1)
                                }
                                }
                            }
                        }
                        }
                        Label {
                            width: parent.width
                            text: (section.features.length === 1 ? qsTr("1 privilegio attivo") : qsTr("%1 privilegi attivi").arg(section.features.length))
                                + (section.pending.length ? " · " + (section.pending.length === 1 ? qsTr("1 scelta da completare") : qsTr("%1 scelte da completare").arg(section.pending.length)) : "")
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: "#536976"
                        }
                        Flow {
                            width: parent.width
                            spacing: 8
                            Button {
                                objectName: "classLevelUp_" + section.modelData.type
                                text: qsTr("+ Livello")
                                onClicked: root.addLevelRequested(section.modelData.type)
                            }
                            Button {
                                objectName: "classLevelDown_" + section.modelData.type
                                text: qsTr("− Livello")
                                enabled: section.modelData.level > 0
                                onClicked: root.removeLevelRequested(section.modelData.type)
                            }
                        }
                    }
                }
                Column {
                    visible: !!section.modelData.archetypes?.length
                    width: parent.width
                    spacing: 8
                    Label { text: qsTr("Archetipi"); font.pixelSize: 18; font.bold: true; color: "#253947" }
                    Repeater {
                        model: section.modelData.archetypes ?? []
                        Column {
                            id: selectedArchetype
                            required property var modelData
                            width: section.width
                            spacing: 4
                        InfoDetails {
                            objectName: "classArchetype_" + section.modelData.type + "_" + selectedArchetype.modelData.id
                            width: section.width
                            rows: [{kind: InfoDetails.Heading, text: selectedArchetype.modelData.name}, {kind: InfoDetails.Entry, text: selectedArchetype.modelData.description}]
                        }
                        Button {
                            objectName: "removeArchetype_" + section.modelData.type + "_" + selectedArchetype.modelData.id
                            text: qsTr("× Rimuovi archetipo")
                            onClicked: root.removeArchetypeRequested(section.modelData.type, selectedArchetype.modelData.id)
                        }
                        }
                    }
                }
                Column {
                    width: parent.width
                    spacing: 12
                    Flow {
                        width: parent.width
                        spacing: 12
                        Label { text: qsTr("Privilegi ottenuti"); font.pixelSize: 20; font.bold: true; color: "#253947" }
                        Button {
                            objectName: "chooseArchetype_" + section.modelData.type
                            text: qsTr("Archetipi")
                            onClicked: { root.editingArchetypeClass = section.modelData; archetypePicker.open(); }
                        }
                    }
                    Label { visible: !section.features.length; text: qsTr("Nessun privilegio attivo."); color: "#777777" }
                    GridLayout {
                        id: featureGrid
                        objectName: "classFeatureGrid_" + section.modelData.type
                        width: parent.width
                        columns: width >= 620 ? 2 : 1
                        columnSpacing: 12
                        rowSpacing: 12
                        Repeater {
                            model: section.features
                            Rectangle {
                                id: feature
                                required property var modelData
                                objectName: "classFeature_" + section.modelData.type + "_" + modelData.id
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: (featureGrid.width - (featureGrid.columns - 1) * featureGrid.columnSpacing) / featureGrid.columns
                                implicitHeight: Math.max(134, featureContent.height + 36)
                                color: "#fafbfc"
                                border.color: "#d2dbe1"
                                radius: 7
                                Column {
                                    id: featureContent
                                    x: 18; y: 18
                                    width: parent.width - 36
                                    spacing: 10
                                    Label { width: parent.width; text: feature.modelData.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 18; font.bold: true; color: "#253947" }
                                    Label { width: parent.width; text: root.levelText(feature.modelData); wrapMode: Text.Wrap; font.pixelSize: 12; color: "#687c88" }
                                    Label {
                                        objectName: "classFeatureDescription_" + section.modelData.type + "_" + feature.modelData.id
                                        width: parent.width
                                        text: feature.modelData.description || qsTr("Nessuna descrizione disponibile.")
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 15
                                        lineHeight: 1.2
                                        color: "#4b5962"
                                    }
                                    Repeater {
                                        model: root.choicesFor(feature.modelData)
                                        Column {
                                            id: selectedChoice
                                            required property var modelData
                                            width: featureContent.width
                                            spacing: 4
                                            Label { width: parent.width; text: selectedChoice.modelData.choice.prompt; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 12; color: "#687c88" }
                                            Button {
                                                objectName: "editClassChoice_" + section.modelData.type + "_" + selectedChoice.modelData.acquisition.id + "_" + selectedChoice.modelData.choice.id
                                                text: "⇄"
                                                Accessible.name: qsTr("Modifica scelta")
                                                onClicked: root.editChoice(section.modelData.type, selectedChoice.modelData.acquisition.id, selectedChoice.modelData.choice)
                                            }
                                            Repeater {
                                                model: selectedChoice.modelData.selected
                                                Column {
                                                    id: selectedOption
                                                    required property var modelData
                                                    width: featureContent.width
                                                    spacing: 3
                                                    Label { width: parent.width; text: selectedOption.modelData.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.bold: true; font.pixelSize: 14; color: "#365f75" }
                                                    Label { visible: !!text; width: parent.width; text: selectedOption.modelData.description ?? ""; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 13; color: "#687c88" }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Column {
                    visible: section.pending.length > 0
                    width: parent.width
                    spacing: 12
                    Label { text: qsTr("Scelte di classe"); font.pixelSize: 20; font.bold: true; color: "#253947" }
                    Repeater {
                        model: section.pending
                        Rectangle {
                            id: pendingChoice
                            required property var modelData
                            objectName: "classPendingChoice_" + section.modelData.type + "_" + modelData.acquisition.id + "_" + modelData.choice.id
                            width: section.width
                            implicitHeight: pendingContent.height + 36
                            color: "#fffaf0"
                            border.color: "#dfcba6"
                            radius: 7
                            Column {
                                id: pendingContent
                                x: 18; y: 18
                                width: parent.width - 36
                                spacing: 10
                                Label { width: parent.width; text: pendingChoice.modelData.featureName + " · " + qsTr("Livello %1").arg(pendingChoice.modelData.acquisition.level); textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 12; color: "#997339" }
                                Label { width: parent.width; text: pendingChoice.modelData.choice.prompt; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 18; font.bold: true; color: "#594527" }
                                Label {
                                    width: parent.width
                                    text: qsTr("Da scegliere ancora: %1").arg(pendingChoice.modelData.choice.remainingSelections)
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 14
                                    color: "#786347"
                                }
                                Label { width: parent.width; text: pendingChoice.modelData.choice.blocksAdvancement ? qsTr("Da completare prima di avanzare di livello.") : qsTr("Non blocca l’avanzamento di livello."); wrapMode: Text.Wrap; font.pixelSize: 12; font.italic: true; color: "#94764e" }
                                Button {
                                    objectName: "completeClassChoice_" + section.modelData.type + "_" + pendingChoice.modelData.acquisition.id + "_" + pendingChoice.modelData.choice.id
                                    text: qsTr("Completa scelta…")
                                    onClicked: root.editChoice(section.modelData.type, pendingChoice.modelData.acquisition.id, pendingChoice.modelData.choice)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
