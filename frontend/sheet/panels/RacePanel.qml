pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var race
    property var catalog: []
    property var alternateFeatures: []
    signal setRaceRequested(string raceId)
    signal selectAlternateTraitRequested(string featureId)
    signal removeAlternateTraitRequested(string featureId)
    signal changeChoiceRequested(string elementId, string choiceId, var optionIds)
    property string editingElementId: ""
    property var editingChoice: null
    property var draftOptionIds: []
    readonly property var elements: race?.race ? race.activeElements ?? [] : []
    readonly property var pending: elements.reduce((entries, element) => entries.concat(
        (element.missingChoices ?? []).map(choice => ({elementId: element.id, elementName: element.name, choice: choice}))), [])
    implicitWidth: 900
    implicitHeight: content.height
    onVisibleChanged: if (!visible) { racePicker.close(); alternatePicker.close(); choicePicker.close(); }
    onRaceChanged: {
        if (alternatePicker) alternatePicker.close();
        if (choicePicker) choicePicker.close();
    }

    function editChoice(elementId, choice) {
        editingElementId = elementId;
        editingChoice = choice;
        draftOptionIds = (choice.selectedOptions ?? []).map(option => option.id);
        choicePicker.open();
    }

    Dialog {
        id: choicePicker
        parent: Overlay.overlay
        modal: true
        title: root.editingChoice?.selectedOptions?.length ? qsTr("Modifica scelta") : qsTr("Completa scelta")
        width: Math.min(520, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: choicePicker.reject() }
            Button {
                objectName: "confirmRacialChoice"
                text: qsTr("Conferma")
                enabled: root.editingChoice?.selectionCount > 0 && root.draftOptionIds.length === root.editingChoice.selectionCount
                    && root.draftOptionIds.every(id => (root.editingChoice.options ?? []).some(option => option.id === id))
                onClicked: {
                    root.changeChoiceRequested(root.editingElementId, root.editingChoice.id, root.draftOptionIds.slice());
                    choicePicker.close();
                }
            }
        }
        contentItem: Column {
            spacing: 10
            Label {
                width: parent.width
                text: root.editingChoice?.prompt ?? ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.bold: true
            }
            Label {
                width: parent.width
                text: qsTr("Selezionate: %1 / %2").arg(root.draftOptionIds.length).arg(root.editingChoice?.selectionCount ?? 0)
                color: "#687c88"
            }
            ListView {
                id: choiceList
                objectName: "racialChoiceList"
                width: parent.width
                height: Math.min(300, Math.max(48, count * 42), (choicePicker.parent?.height ?? 600) * 0.45)
                model: root.editingChoice?.options ?? []
                clip: true
                ScrollBar.vertical: ScrollBar {}
                delegate: CheckBox {
                    id: choiceOption
                    required property var modelData
                    objectName: "racialChoiceOption_" + modelData.id
                    width: choiceList.width
                    height: 42
                    text: modelData.name
                    checked: root.draftOptionIds.includes(modelData.id)
                    enabled: root.draftOptionIds.includes(modelData.id) || root.draftOptionIds.length < (root.editingChoice?.selectionCount ?? 0)
                    contentItem: Label {
                        text: choiceOption.text
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        leftPadding: choiceOption.indicator.width + choiceOption.spacing
                        color: choiceOption.enabled ? "#253947" : "#999999"
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.draftOptionIds = checked
                        ? [...root.draftOptionIds.filter(id => id !== modelData.id), modelData.id]
                        : root.draftOptionIds.filter(id => id !== modelData.id)
                }
            }
        }
    }

    Dialog {
        id: alternatePicker
        parent: Overlay.overlay
        modal: true
        title: qsTr("Tratti alternativi · %1").arg(root.race?.race?.name ?? "")
        width: Math.min(520, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        onAboutToShow: alternateSearch.clear()
        onOpened: alternateSearch.forceActiveFocus()
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: alternatePicker.reject() }
        }
        contentItem: Column {
            spacing: 10
            TextField {
                id: alternateSearch
                objectName: "alternateTraitSearch"
                width: parent.width
                placeholderText: qsTr("Cerca un tratto alternativo…")
                Accessible.name: placeholderText
                selectByMouse: true
                ContextMenu.menu: null
            }
            ListView {
                id: alternateList
                objectName: "alternateTraitList"
                width: parent.width
                height: Math.min(360, (alternatePicker.parent?.height ?? 600) * 0.5)
                clip: true
                model: root.race?.race ? (root.alternateFeatures ?? []).filter(entry => entry.name.toLocaleLowerCase().includes(alternateSearch.text.trim().toLocaleLowerCase()))
                    .slice().sort((a, b) => a.name.localeCompare(b.name, "it") || a.id.localeCompare(b.id)) : []
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: alternateCandidate
                    required property var modelData
                    width: alternateList.width
                    highlighted: root.elements.some(element => element.id === modelData.id && element.removable === true)
                    enabled: !highlighted
                    contentItem: Label {
                        text: alternateCandidate.modelData.name + (alternateCandidate.highlighted ? qsTr(" · selezionato") : "")
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: alternateCandidate.highlighted ? "#52768e" : "#253947"
                        font.pixelSize: 15
                    }
                    onClicked: {
                        root.selectAlternateTraitRequested(modelData.id);
                        alternatePicker.close();
                    }
                }
            }
            Label {
                visible: alternateList.count === 0
                width: parent.width
                text: root.alternateFeatures?.length ? qsTr("Nessun tratto corrispondente.") : qsTr("Nessun tratto alternativo disponibile.")
                wrapMode: Text.Wrap
                color: "#777777"
            }
        }
    }

    Dialog {
        id: racePicker
        objectName: "racePicker"
        parent: Overlay.overlay
        modal: true
        title: qsTr("Scegli la razza")
        width: Math.min(520, (parent?.width ?? root.width) - 24)
        x: ((parent?.width ?? root.width) - width) / 2
        y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
        footer: DialogButtonBox {
            Button {
                text: qsTr("Annulla")
                onClicked: racePicker.reject()
            }
        }
        onAboutToShow: raceSearch.clear()
        onOpened: raceSearch.forceActiveFocus()
        contentItem: Column {
            spacing: 10
            TextField {
                id: raceSearch
                objectName: "raceSearch"
                width: parent.width
                placeholderText: qsTr("Cerca una razza…")
                Accessible.name: placeholderText
                selectByMouse: true
                ContextMenu.menu: null
            }
            ListView {
                id: raceList
                objectName: "raceCatalogList"
                width: parent.width
                height: Math.min(360, (racePicker.parent?.height ?? 600) * 0.5)
                clip: true
                model: (root.catalog ?? []).filter(entry => entry.name.toLocaleLowerCase().includes(raceSearch.text.trim().toLocaleLowerCase()))
                    .slice().sort((a, b) => a.name.localeCompare(b.name, "it") || a.id.localeCompare(b.id))
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: candidate
                    required property var modelData
                    objectName: "raceCandidate_" + modelData.id
                    width: raceList.width
                    highlighted: root.race?.race?.id === modelData.id
                    enabled: !highlighted
                    contentItem: Label {
                        text: candidate.modelData.name + (candidate.highlighted ? qsTr(" · attuale") : "")
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: candidate.highlighted ? "#52768e" : "#253947"
                        font.pixelSize: 15
                    }
                    onClicked: {
                        root.setRaceRequested(modelData.id);
                        racePicker.close();
                    }
                }
            }
            Label {
                visible: raceList.count === 0
                width: parent.width
                text: root.catalog?.length ? qsTr("Nessuna razza corrispondente.") : qsTr("Catalogo non disponibile.")
                wrapMode: Text.Wrap
                color: "#777777"
            }
        }
    }

    Column {
        id: content
        width: root.width
        spacing: 20
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
                Label { text: qsTr("RAZZA"); font.pixelSize: 11; font.letterSpacing: 2; font.bold: true; color: "#527080" }
                GridLayout {
                    width: parent.width
                    columns: width >= 480 ? 2 : 1
                    columnSpacing: 16
                    rowSpacing: 10
                    Label {
                        objectName: "raceName"
                        Layout.fillWidth: true
                        text: root.race?.race?.name ?? qsTr("Nessuna razza selezionata")
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.pixelSize: 32
                        font.bold: true
                        color: "#20333f"
                    }
                    Flow {
                        Layout.fillWidth: hero.width < 480
                        Layout.preferredWidth: changeRaceButton.implicitWidth + alternateButton.implicitWidth + 8
                        Layout.maximumWidth: hero.width
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
                        Button {
                            id: changeRaceButton
                            objectName: "changeRace"
                            readonly property alias pickerPopup: racePicker
                            text: root.race?.race ? qsTr("Cambia razza") : qsTr("Scegli razza")
                            onClicked: racePicker.open()
                        }
                        Button {
                            id: alternateButton
                            objectName: "selectAlternateTraits"
                            readonly property alias pickerPopup: alternatePicker
                            text: qsTr("Tratti alternativi")
                            enabled: !!root.race?.race
                            onClicked: alternatePicker.open()
                        }
                    }
                }
                Label {
                    width: parent.width
                    visible: !!root.race?.race
                    text: (root.elements.length === 1 ? qsTr("1 tratto attivo") : qsTr("%1 tratti attivi").arg(root.elements.length))
                        + (root.pending.length ? " · " + (root.pending.length === 1 ? qsTr("1 scelta da completare") : qsTr("%1 scelte da completare").arg(root.pending.length)) : "")
                    wrapMode: Text.Wrap
                    color: "#536976"
                    font.pixelSize: 14
                }
            }
        }
        Column {
            visible: !!root.race?.race
            width: parent.width
            spacing: 12
            Label { text: qsTr("Tratti razziali"); font.pixelSize: 20; font.bold: true; color: "#253947" }
            Label { visible: !root.elements.length; text: qsTr("Nessun tratto attivo."); color: "#777777" }
            GridLayout {
                id: traitsGrid
                objectName: "racialTraitsGrid"
                width: parent.width
                columns: width >= 620 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12
                Repeater {
                    model: root.elements
                    Rectangle {
                        id: trait
                        required property var modelData
                        objectName: "racialTrait_" + modelData.id
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: (traitsGrid.width - (traitsGrid.columns - 1) * traitsGrid.columnSpacing) / traitsGrid.columns
                        implicitHeight: Math.max(134, traitContent.height + 36)
                        color: "#fafbfc"
                        border.color: "#d2dbe1"
                        radius: 7
                        Column {
                            id: traitContent
                            x: 18; y: 18
                            width: parent.width - 36
                            spacing: 10
                            Row {
                                width: parent.width
                                spacing: trait.modelData.removable === true ? 6 : 0
                                Flow {
                                    id: traitHeading
                                    width: parent.width - (trait.modelData.removable === true ? 34 : 0)
                                    spacing: 6
                                    Label {
                                        id: traitTitle
                                        objectName: "racialTitle_" + trait.modelData.id
                                        width: Math.min(implicitWidth, traitHeading.width)
                                        text: trait.modelData.name
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: "#253947"
                                    }
                                    Label {
                                        objectName: "racialAlternateLabel_" + trait.modelData.id
                                        visible: trait.modelData.removable === true
                                        width: Math.min(implicitWidth, traitHeading.width)
                                        topPadding: 4
                                        text: qsTr("Tratto Alternativo")
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "#687c88"
                                    }
                                }
                                ToolButton {
                                    id: removeTrait
                                    objectName: "removeRacialTrait_" + trait.modelData.id
                                    visible: trait.modelData.removable === true
                                    width: 28
                                    height: Math.max(28, traitHeading.implicitHeight)
                                    Accessible.name: qsTr("Rimuovi tratto alternativo: ") + trait.modelData.name
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Rimuovi tratto alternativo")
                                    background: Item {}
                                    contentItem: Label {
                                        text: "×"
                                        font.pixelSize: 22
                                        color: removeTrait.hovered ? "#b33434" : "#777777"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: root.removeAlternateTraitRequested(trait.modelData.id)
                                }
                            }
                            Label {
                                objectName: "racialDescription_" + trait.modelData.id
                                width: parent.width
                                text: trait.modelData.description || qsTr("Nessuna descrizione disponibile.")
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 15
                                lineHeight: 1.2
                                color: "#4b5962"
                            }
                            Repeater {
                                model: trait.modelData.choices ?? []
                                Column {
                                    id: selected
                                    required property var modelData
                                    width: traitContent.width
                                    spacing: 4
                                    Label { width: parent.width; text: selected.modelData.prompt; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 12; color: "#687c88" }
                                    Row {
                                        width: parent.width
                                        spacing: 6
                                        Label {
                                            id: selectedNames
                                            objectName: "racialSelected_" + trait.modelData.id + "_" + selected.modelData.id
                                            width: parent.width - 38
                                            height: Math.max(32, implicitHeight)
                                            text: (selected.modelData.selectedOptions ?? []).map(option => option.name).join(" · ")
                                            textFormat: Text.PlainText
                                            wrapMode: Text.Wrap
                                            verticalAlignment: Text.AlignVCenter
                                            font.bold: true
                                            font.pixelSize: 14
                                            color: "#365f75"
                                        }
                                        ToolButton {
                                            id: changeChoice
                                            objectName: "changeRacialChoice_" + trait.modelData.id + "_" + selected.modelData.id
                                            readonly property var pickerPopup: choicePicker
                                            width: 32
                                            height: selectedNames.height
                                            Accessible.name: qsTr("Cambia scelta: ") + selected.modelData.prompt
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Cambia scelta")
                                            background: Rectangle {
                                                color: changeChoice.hovered || changeChoice.visualFocus ? "#e6edf2" : "transparent"
                                                radius: 4
                                            }
                                            contentItem: Label {
                                                text: "⇄"
                                                font.pixelSize: 23
                                                color: "#52768e"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: root.editChoice(trait.modelData.id, selected.modelData)
                                        }
                                    }
                                }
                            }
                            Label {
                                objectName: "racialPending_" + trait.modelData.id
                                visible: !!trait.modelData.missingChoices?.length
                                text: qsTr("Scelta da completare ↓")
                                color: "#9b661d"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
        Column {
            visible: root.pending.length > 0
            width: parent.width
            spacing: 12
            Label { text: qsTr("Scelte razziali"); font.pixelSize: 20; font.bold: true; color: "#253947" }
            Repeater {
                model: root.pending
                Rectangle {
                    id: pendingChoice
                    required property var modelData
                    objectName: "racialChoice_" + modelData.elementId + "_" + modelData.choice.id
                    width: content.width
                    implicitHeight: choiceContent.height + 36
                    color: "#fffaf0"
                    border.color: "#dfcba6"
                    radius: 7
                    Column {
                        id: choiceContent
                        x: 18; y: 18
                        width: parent.width - 36
                        spacing: 10
                        Label { width: parent.width; text: pendingChoice.modelData.elementName; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 12; color: "#997339" }
                        Label {
                            width: parent.width
                            text: pendingChoice.modelData.choice.prompt
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.pixelSize: 18
                            font.bold: true
                            color: "#594527"
                        }
                        Label {
                            objectName: "racialChoiceCount_" + pendingChoice.modelData.elementId + "_" + pendingChoice.modelData.choice.id
                            width: parent.width
                            text: qsTr("Opzioni da scegliere: %1").arg(pendingChoice.modelData.choice.selectionCount)
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: "#786347"
                        }
                        Button {
                            objectName: "completeRacialChoice_" + pendingChoice.modelData.elementId + "_" + pendingChoice.modelData.choice.id
                            readonly property var pickerPopup: choicePicker
                            text: qsTr("Completa scelta…")
                            enabled: pendingChoice.modelData.choice.selectionCount > 0
                                && (pendingChoice.modelData.choice.options?.length ?? 0) >= pendingChoice.modelData.choice.selectionCount
                            onClicked: root.editChoice(pendingChoice.modelData.elementId, pendingChoice.modelData.choice)
                        }
                        Label {
                            objectName: "racialChoicePolicy_" + pendingChoice.modelData.elementId + "_" + pendingChoice.modelData.choice.id
                            width: parent.width
                            text: pendingChoice.modelData.choice.blocksAdvancement ? qsTr("Da completare prima di avanzare di livello.") : qsTr("Non blocca l’avanzamento di livello.")
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                            font.italic: true
                            color: "#94764e"
                        }
                    }
                }
            }
        }
    }
}
