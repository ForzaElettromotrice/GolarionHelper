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
    signal identityChangeRequested(var proposedIdentity)

    property var draft: ({})
    property string baselineDraft: ""
    property bool initialized: false
    property bool externalChangePending: false
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

    onIdentityChanged: {
        if (!initialized) return;
        const incoming = makeDraft(identity);
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
        }
    }

    ColumnLayout {
        id: content
        width: root.availableWidth
        spacing: 16

        Label { text: qsTr("Identità"); font.pixelSize: 24; font.bold: true }
        Label {
            text: qsTr("I campi possono restare vuoti. L'allineamento non scelto bloccherà l'avanzamento di livello.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

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
                    onActivated: root.edit("alignment", currentValue)
                }
            }
            Field { objectName: "deity"; caption: qsTr("Divinità"); text: root.draft.deity ?? ""; onEdited: value => root.edit("deity", value) }
            Field { objectName: "homeland"; caption: qsTr("Provenienza"); text: root.draft.homeland ?? ""; onEdited: value => root.edit("homeland", value) }
            Field { objectName: "gender"; caption: qsTr("Genere"); text: root.draft.gender ?? ""; onEdited: value => root.edit("gender", value) }
        }

        Label { text: qsTr("Aspetto e dati fisici"); font.bold: true }
        GridLayout {
            columns: root.availableWidth >= 560 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 12
            Layout.fillWidth: true
            Field { objectName: "age"; caption: qsTr("Età (anni)"); text: root.draft.age ?? ""; inputMethodHints: Qt.ImhDigitsOnly; onEdited: value => root.edit("age", value) }
            Field { objectName: "height"; caption: qsTr("Altezza (cm)"); text: root.draft.heightCentimeters ?? ""; inputMethodHints: Qt.ImhDigitsOnly; onEdited: value => root.edit("heightCentimeters", value) }
            Field { objectName: "weight"; caption: qsTr("Peso (kg)"); text: root.draft.weightKilograms ?? ""; inputMethodHints: Qt.ImhFormattedNumbersOnly; onEdited: value => root.edit("weightKilograms", value) }
            Field { objectName: "hair"; caption: qsTr("Capelli"); text: root.draft.hair ?? ""; onEdited: value => root.edit("hair", value) }
            Field { objectName: "eyes"; caption: qsTr("Occhi"); text: root.draft.eyes ?? ""; onEdited: value => root.edit("eyes", value) }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label { text: qsTr("Descrizione dell'aspetto") }
            TextArea {
                ContextMenu.menu: null
                objectName: "appearanceInput"
                Layout.fillWidth: true
                implicitHeight: Math.max(100, contentHeight + topPadding + bottomPadding)
                text: root.draft.appearance ?? ""
                readOnly: !root.editable
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                Accessible.name: qsTr("Descrizione dell'aspetto")
                onTextChanged: {
                    if (activeFocus && text !== root.draft.appearance) root.edit("appearance", text);
                }
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
