pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    // UI vocabulary; the future adapter must explicitly map the domain enum.
    enum Alignment {
        Unspecified,
        LawfulGood, NeutralGood, ChaoticGood,
        LawfulNeutral, Neutral, ChaoticNeutral,
        LawfulEvil, NeutralEvil, ChaoticEvil
    }

    required property var identity
    property bool readOnly: false
    property bool busy: false
    property string errorMessage: ""
    property bool commitOnEnter: false
    property bool showTitle: true
    signal identityChangeRequested(var proposedIdentity)
    signal identityFieldChangeRequested(string field, var value)

    property var draft: ({})
    property string baselineDraft: ""
    property bool initialized: false
    property bool externalChangePending: false
    property Item focusedEditor: null
    readonly property bool dirty: initialized && JSON.stringify(draft) !== baselineDraft
    readonly property bool editable: !readOnly && !busy
    readonly property string validationMessage: {
        if (!validInteger(draft.age, true))
            return qsTr("L'età deve essere un numero intero non negativo.");
        if (!validInteger(draft.heightCentimeters, false))
            return qsTr("L'altezza deve essere un numero intero positivo, in centimetri.");
        if (!validWeight(draft.weightKilograms))
            return qsTr("Il peso deve essere positivo, in kg, con al massimo tre decimali.");
        if (!Number.isInteger(draft.alignment) || draft.alignment < IdentityPanel.Unspecified
                || draft.alignment > IdentityPanel.ChaoticEvil)
            return qsTr("Seleziona un allineamento valido.");
        return "";
    }
    readonly property bool valid: initialized && validationMessage.length === 0
    readonly property bool canApply: editable && dirty && valid && !externalChangePending

    padding: 20
    implicitWidth: 700
    implicitHeight: content.implicitHeight + topPadding + bottomPadding

    function optionalText(value) {
        const trimmed = String(value ?? "").trim();
        return trimmed.length === 0 ? null : trimmed;
    }

    function validInteger(value, allowZero) {
        if (value === "" || value === undefined) return true;
        return /^\d+$/.test(value) && Number(value) <= 2147483647
                && (allowZero || Number(value) > 0);
    }

    function validWeight(value) {
        if (value === "" || value === undefined) return true;
        if (!/^\d+(?:[.,]\d{1,3})?$/.test(value)) return false;
        const grams = weightGrams(value);
        return Number.isSafeInteger(grams) && grams > 0;
    }

    function weightGrams(value) {
        const parts = String(value).replace(",", ".").split(".");
        return Number(parts[0]) * 1000 + Number((parts[1] ?? "").padEnd(3, "0"));
    }

    function weightText(value) {
        if (value === null || value === undefined) return "";
        const whole = Math.floor(value / 1000);
        const fraction = String(value % 1000).padStart(3, "0").replace(/0+$/, "");
        return String(whole) + (fraction.length > 0 ? "." + fraction : "");
    }

    function makeDraft(value) {
        return {
            name: value.name ?? "", playerName: value.playerName ?? "",
            alignment: value.alignment ?? IdentityPanel.Unspecified,
            deity: value.deity ?? "", homeland: value.homeland ?? "",
            gender: value.gender ?? "", age: value.age === null || value.age === undefined ? "" : String(value.age),
            heightCentimeters: value.heightCentimeters === null || value.heightCentimeters === undefined ? "" : String(value.heightCentimeters),
            weightKilograms: weightText(value.weightGrams),
            hair: value.hair ?? "", eyes: value.eyes ?? "", appearance: value.appearance ?? ""
        };
    }

    function proposal() {
        return {
            name: String(draft.name ?? "").trim(), playerName: String(draft.playerName ?? "").trim(),
            alignment: draft.alignment,
            deity: optionalText(draft.deity), homeland: optionalText(draft.homeland),
            gender: optionalText(draft.gender), age: draft.age === "" ? null : Number(draft.age),
            heightCentimeters: draft.heightCentimeters === "" ? null : Number(draft.heightCentimeters),
            weightGrams: draft.weightKilograms === "" ? null : weightGrams(draft.weightKilograms),
            hair: optionalText(draft.hair), eyes: optionalText(draft.eyes), appearance: optionalText(draft.appearance)
        };
    }

    function resetDraft() {
        draft = makeDraft(identity);
        baselineDraft = JSON.stringify(draft);
        externalChangePending = false;
    }

    function edit(field, value) {
        if (!editable) return;
        const next = Object.assign({}, draft);
        next[field] = value;
        draft = next;
    }

    function applyChanges() {
        if (canApply) identityChangeRequested(proposal());
    }
    function commitField(field) {
        if (!commitOnEnter || !editable) return false;
        if ((field === "age" && !validInteger(draft.age, true))
                || (field === "heightCentimeters" && !validInteger(draft.heightCentimeters, false))
                || (field === "weightKilograms" && !validWeight(draft.weightKilograms))) return false;
        const key = field === "weightKilograms" ? "weightGrams" : field;
        const value = proposal()[key];
        const baseline = makeDraft(identity);
        if (makeDraft(proposal())[field] !== baseline[field]) identityFieldChangeRequested(key, value);
        return true;
    }
    function trackFocus(editor) {
        if (editor.activeFocus) focusedEditor = editor;
        else if (focusedEditor === editor) focusedEditor = null;
    }

    MouseArea {
        parent: root.Window.window ? root.Window.window.contentItem : null
        anchors.fill: parent
        z: 100000
        enabled: root.visible && root.focusedEditor !== null && !alignmentInput.popup.visible
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onPressed: mouse => {
            const editor = root.focusedEditor;
            mouse.accepted = false;
            if (editor && !editor.contains(editor.mapFromItem(parent, mouse.x, mouse.y))) editor.focus = false;
        }
    }

    onIdentityChanged: {
        if (!initialized) return;
        const incoming = makeDraft(identity);
        if (commitOnEnter) {
            const previous = JSON.parse(baselineDraft);
            const normalized = makeDraft(proposal());
            const next = Object.assign({}, draft);
            for (const field of Object.keys(incoming))
                if (draft[field] === previous[field] || incoming[field] === normalized[field])
                    next[field] = incoming[field];
            draft = next;
            baselineDraft = JSON.stringify(incoming);
            return;
        }
        if (!dirty || JSON.stringify(incoming) === JSON.stringify(makeDraft(proposal()))) {
            resetDraft();
        } else if (JSON.stringify(incoming) !== baselineDraft) {
            externalChangePending = true;
        }
    }
    Component.onCompleted: {
        resetDraft();
        initialized = true;
    }

    component Field: ColumnLayout {
        id: field
        required property string caption
        property string fieldKey: objectName
        property alias text: editor.text
        property alias inputMethodHints: editor.inputMethodHints
        signal edited(string value)
        spacing: 4
        Layout.fillWidth: true
        Label { text: field.caption }
        TextField {
            ContextMenu.menu: null
            id: editor
            objectName: field.objectName + "Input"
            Layout.fillWidth: true
            readOnly: !root.editable
            selectByMouse: true
            Accessible.name: field.caption
            onTextEdited: field.edited(text)
            onActiveFocusChanged: root.trackFocus(editor)
            Keys.onEscapePressed: focus = false
            onAccepted: if (root.commitField(field.fieldKey)) focus = false
        }
    }

    ColumnLayout {
        id: content
        width: root.availableWidth
        spacing: 16

        Label { visible: root.showTitle; text: qsTr("Identità"); font.pixelSize: 24; font.bold: true }

        GridLayout {
            columns: root.availableWidth >= 560 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 12
            Layout.fillWidth: true

            Field { objectName: "name"; caption: qsTr("Nome del personaggio"); text: root.draft.name ?? ""; onEdited: value => root.edit("name", value) }
            Field { objectName: "playerName"; caption: qsTr("Giocatore"); text: root.draft.playerName ?? ""; onEdited: value => root.edit("playerName", value) }
            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Label { text: qsTr("Allineamento") }
                ComboBox {
                    id: alignmentInput
                    objectName: "alignmentInput"
                    Layout.fillWidth: true
                    enabled: root.editable
                    Accessible.name: qsTr("Allineamento")
                    textRole: "label"
                    valueRole: "value"
                    model: [
                        { label: qsTr("Non scelto"), value: IdentityPanel.Unspecified },
                        { label: qsTr("Legale Buono"), value: IdentityPanel.LawfulGood },
                        { label: qsTr("Neutrale Buono"), value: IdentityPanel.NeutralGood },
                        { label: qsTr("Caotico Buono"), value: IdentityPanel.ChaoticGood },
                        { label: qsTr("Legale Neutrale"), value: IdentityPanel.LawfulNeutral },
                        { label: qsTr("Neutrale"), value: IdentityPanel.Neutral },
                        { label: qsTr("Caotico Neutrale"), value: IdentityPanel.ChaoticNeutral },
                        { label: qsTr("Legale Malvagio"), value: IdentityPanel.LawfulEvil },
                        { label: qsTr("Neutrale Malvagio"), value: IdentityPanel.NeutralEvil },
                        { label: qsTr("Caotico Malvagio"), value: IdentityPanel.ChaoticEvil }
                    ]
                    currentIndex: indexOfValue(root.draft.alignment ?? IdentityPanel.Unspecified)
                    onActiveFocusChanged: root.trackFocus(alignmentInput)
                    Keys.onEscapePressed: event => {
                        if (popup.visible) event.accepted = false;
                        else focus = false;
                    }
                    onActivated: {
                        root.edit("alignment", currentValue);
                        root.commitField("alignment");
                    }
                    Connections {
                        target: alignmentInput.popup
                        function onClosed() { alignmentInput.focus = false; }
                    }
                }
            }
            Field { objectName: "homeland"; caption: qsTr("Provenienza"); text: root.draft.homeland ?? ""; onEdited: value => root.edit("homeland", value) }
            Field { objectName: "gender"; caption: qsTr("Genere"); text: root.draft.gender ?? ""; onEdited: value => root.edit("gender", value) }
        }

        Label { text: qsTr("Aspetto e dati fisici"); font.bold: true }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Field { objectName: "age"; caption: qsTr("Età (anni)"); Layout.preferredWidth: 70; Layout.maximumWidth: 90; text: root.draft.age ?? ""; inputMethodHints: Qt.ImhDigitsOnly; onEdited: value => root.edit("age", value) }
            Field { objectName: "height"; fieldKey: "heightCentimeters"; caption: qsTr("Altezza (cm)"); Layout.preferredWidth: 90; Layout.maximumWidth: 100; text: root.draft.heightCentimeters ?? ""; inputMethodHints: Qt.ImhDigitsOnly; onEdited: value => root.edit("heightCentimeters", value) }
            Field { objectName: "weight"; fieldKey: "weightKilograms"; caption: qsTr("Peso (kg)"); Layout.preferredWidth: 85; Layout.maximumWidth: 100; text: root.draft.weightKilograms ?? ""; inputMethodHints: Qt.ImhFormattedNumbersOnly; onEdited: value => root.edit("weightKilograms", value) }
            Item { Layout.fillWidth: true }
        }
        GridLayout {
            columns: root.availableWidth >= 560 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 12
            Layout.fillWidth: true
            Field { objectName: "hair"; caption: qsTr("Capelli"); text: root.draft.hair ?? ""; onEdited: value => root.edit("hair", value) }
            Field { objectName: "eyes"; caption: qsTr("Occhi"); text: root.draft.eyes ?? ""; onEdited: value => root.edit("eyes", value) }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label { text: qsTr("Descrizione dell'aspetto") }
            TextArea {
                id: appearanceInput
                ContextMenu.menu: null
                objectName: "appearanceInput"
                background: InfoFrame {}
                Layout.fillWidth: true
                implicitHeight: Math.max(100, contentHeight + topPadding + bottomPadding)
                text: root.draft.appearance ?? ""
                readOnly: !root.editable
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                Accessible.name: qsTr("Descrizione dell'aspetto")
                onActiveFocusChanged: root.trackFocus(appearanceInput)
                Keys.onEscapePressed: focus = false
                onTextChanged: {
                    if (activeFocus && text !== root.draft.appearance) root.edit("appearance", text);
                }
                function handleEnter(event) {
                    if (!root.commitOnEnter || (event.modifiers & Qt.ShiftModifier)) {
                        event.accepted = false;
                        return;
                    }
                    if (root.commitField("appearance")) focus = false;
                    event.accepted = true;
                }
                Keys.onReturnPressed: event => handleEnter(event)
                Keys.onEnterPressed: event => handleEnter(event)
            }
        }
        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.WordWrap
            text: root.externalChangePending ? qsTr("I dati sono cambiati mentre modificavi il pannello. Ricaricali prima di applicare altre modifiche.")
                : root.validationMessage || root.errorMessage
            Accessible.role: Accessible.AlertMessage
        }
        RowLayout {
            Layout.fillWidth: true
            visible: !root.commitOnEnter
            Button {
                objectName: "resetButton"
                text: root.externalChangePending ? qsTr("Ricarica dati") : qsTr("Annulla modifiche")
                enabled: !root.busy && (root.dirty || root.externalChangePending)
                onClicked: root.resetDraft()
            }
            Item { Layout.fillWidth: true }
            Button {
                objectName: "applyButton"
                text: root.busy ? qsTr("Applicazione…") : qsTr("Applica")
                enabled: root.canApply
                onClicked: root.applyChanges()
            }
        }
    }
}
