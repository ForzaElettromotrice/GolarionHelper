pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: root
    enum Origin { Intelligence, Linguistics, Extra, CustomLinguistics }
    required property var languages
    property bool prominentHeading: false
    property var languageNames: ({})
    property var extraLanguageIds: []
    signal addRequested(string languageId, int origin)
    signal removeRequested(string languageId)
    signal grantRequested(string languageId)
    signal customLearnRequested(string name)
    signal customGrantRequested(string name)
    implicitWidth: 360
    implicitHeight: content.height

    component CustomLanguageInput: MenuItem {
        id: customInput
        signal submitted(string name)
        property alias textField: editor
        contentItem: TextField {
            id: editor
            placeholderText: qsTr("Lingua personalizzata…")
            Accessible.name: placeholderText
            ContextMenu.menu: null
            selectByMouse: true
            function submit() {
                const name = text.trim();
                if (name.length) customInput.submitted(name);
            }
            Keys.onShortcutOverride: event => {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true;
            }
            Keys.onReturnPressed: submit()
            Keys.onEnterPressed: submit()
        }
    }

    function languageName(id) { return languageNames[id] ?? id; }
    function originName(origin) { return origin === LanguagesPanel.Intelligence ? qsTr("Intelligenza") : qsTr("Linguistica"); }
    function remaining(origin) { return origin === LanguagesPanel.Intelligence ? languages?.remainingIntelligence : languages?.remainingLinguistics; }
    function quotaText(origin) {
        const value = remaining(origin);
        return originName(origin) + ": " + (value == null ? "—" : value < 0 ? qsTr("%1 in eccesso").arg(-value)
            : value === 1 ? qsTr("1 disponibile") : qsTr("%1 disponibili").arg(value));
    }
    function choices(origin) {
        const known = new Set((languages?.spoken ?? []).map(entry => entry.id));
        const ids = (languages?.options ?? []).filter(entry => entry.origin === origin).reduce((all, entry) => all.concat(entry.languageIds ?? []), []);
        return [...new Set(ids)].filter(id => !known.has(id)).sort((a, b) => languageName(a).localeCompare(languageName(b), "it"));
    }
    function groupEntries(learned) {
        return (languages?.spoken ?? []).filter(entry => (entry.learnedFrom != null) === learned)
            .slice().sort((a, b) => languageName(a.id).localeCompare(languageName(b.id), "it"));
    }
    function extraChoices() {
        const known = new Set((languages?.spoken ?? []).map(entry => entry.id));
        return [...new Set(extraLanguageIds ?? [])].filter(id => !known.has(id))
            .sort((a, b) => languageName(a).localeCompare(languageName(b), "it"));
    }
    Column {
        id: content
        width: root.prominentHeading ? root.width : Math.min(root.width, root.implicitWidth)
        spacing: 8
        Row {
            width: parent.width
            height: root.prominentHeading ? 42 : 32
            Label {
                objectName: "sectionHeading"
                width: parent.width - 32
                height: parent.height
                text: qsTr("Lingue")
                font.pixelSize: root.prominentHeading ? 20 : 16
                font.bold: true
                leftPadding: root.prominentHeading ? 12 : 0
                color: root.prominentHeading ? "white" : "#181818"
                verticalAlignment: Text.AlignVCenter
                background: Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: -32
                    color: "#181818"
                    visible: root.prominentHeading
                }
            }
            ToolButton {
                id: extraButton
                objectName: "addExtraLanguage"
                readonly property alias choicesPopup: extraMenu
                width: 32; height: parent.height
                palette.buttonText: root.prominentHeading ? "white" : "#181818"
                text: "+"
                font.pixelSize: 24
                Accessible.name: qsTr("Aggiungi una lingua extra senza consumare posti")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Aggiungi lingua extra")
                onClicked: extraMenu.visible ? extraMenu.close() : extraMenu.open()
                onVisibleChanged: if (!visible) extraMenu.close()
                Menu {
                    id: extraMenu
                    width: content.width
                    x: extraButton.width - width
                    y: extraButton.height
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                    onAboutToShow: extraInput.textField.clear()
                    MenuItem {
                        visible: !root.extraChoices().length
                        height: visible ? implicitHeight : 0
                        enabled: false
                        text: qsTr("Nessuna lingua disponibile")
                    }
                    Instantiator {
                        model: root.extraChoices()
                        delegate: MenuItem {
                            id: extraChoice
                            required property string modelData
                            text: root.languageName(modelData)
                            contentItem: Label { text: extraChoice.text; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                            onTriggered: if (root.extraChoices().includes(modelData)) root.grantRequested(modelData)
                        }
                        onObjectAdded: (index, object) => extraMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => extraMenu.removeItem(object)
                    }
                    CustomLanguageInput {
                        id: extraInput
                        objectName: "customExtraLanguage"
                        onSubmitted: name => {
                            root.customGrantRequested(name);
                            extraMenu.close();
                        }
                    }
                }
            }
        }
        Repeater {
            model: 2
            Column {
                id: group
                required property int index
                readonly property var entries: root.groupEntries(index === 1)
                width: content.width
                spacing: 4
                Label {
                    width: parent.width
                    padding: 8
                    text: group.index === 0 ? qsTr("Concesse") : qsTr("Apprese")
                    font.bold: true
                    font.pixelSize: 14
                    color: "white"
                    background: Rectangle { color: "#181818" }
                }
                Label {
                    visible: !group.entries.length
                    width: parent.width
                    padding: 8
                    text: qsTr("Nessuna")
                    color: "#777777"
                }
                Repeater {
                    model: group.entries
                    Rectangle {
                        id: languageRow
                        required property var modelData
                        readonly property bool removable: modelData.removable ?? (modelData.learnedFrom != null)
                        objectName: "language_" + modelData.id
                        width: group.width
                        implicitHeight: labels.height + 16
                        color: "#f2f5f7"
                        border.color: "#c6cfd5"
                        radius: 4
                        Column {
                            id: labels
                            x: 10; y: 8
                            width: parent.width - (languageRow.removable ? 44 : 20)
                            spacing: 3
                            Label {
                                width: parent.width
                                text: root.languageName(languageRow.modelData.id)
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 15
                                color: "#253947"
                            }
                            Label {
                                width: parent.width
                                text: (languageRow.modelData.learnedFrom != null ? [root.originName(languageRow.modelData.learnedFrom)] : [])
                                    .concat(languageRow.modelData.grantedSources ?? []).join(" · ")
                                visible: text.length > 0
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 12
                                color: "#666666"
                            }
                        }
                        ToolButton {
                            id: removeButton
                            objectName: "removeLanguage_" + languageRow.modelData.id
                            visible: languageRow.removable
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 32; height: 32
                            Accessible.name: qsTr("Rimuovi lingua appresa: ") + root.languageName(languageRow.modelData.id)
                            background: Item {}
                            contentItem: Label { text: "×"; font.pixelSize: 22; color: removeButton.hovered ? "#b33434" : "#777777"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: root.removeRequested(languageRow.modelData.id)
                        }
                    }
                }
            }
        }
        Repeater {
            model: 2
            Column {
                id: originGroup
                required property int index
                width: content.width
                spacing: 6
                Label {
                    objectName: "languageQuota_" + originGroup.index
                    width: parent.width
                    text: root.quotaText(originGroup.index)
                    wrapMode: Text.Wrap
                    color: root.remaining(originGroup.index) < 0 ? "#a34a34" : "#555555"
                    font.pixelSize: 13
                }
                Repeater {
                    model: Math.max(0, root.remaining(originGroup.index) ?? 0)
                    Button {
                        id: slotButton
                        required property int index
                        readonly property alias choicesPopup: languageMenu
                        objectName: "languageSlot_" + originGroup.index + "_" + index
                        width: originGroup.width
                        height: 56
                        Accessible.name: qsTr("Impara una lingua da ") + root.originName(originGroup.index)
                        onClicked: languageMenu.visible ? languageMenu.close() : languageMenu.open()
                        onVisibleChanged: if (!visible) languageMenu.close()
                        contentItem: Label {
                            text: "+"
                            font.pixelSize: 26
                            color: slotButton.hovered || languageMenu.visible ? "#1769aa" : "#68808f"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: slotButton.hovered ? "#edf3f7" : "#f8fafb"
                            border.color: slotButton.visualFocus || languageMenu.visible ? "#52768e" : "#c6cfd5"
                            radius: 4
                            Canvas {
                                anchors.fill: parent
                                anchors.margins: 7
                                onWidthChanged: requestPaint()
                                onHeightChanged: requestPaint()
                                onAvailableChanged: if (available) requestPaint()
                                onPaint: {
                                    const ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.fillStyle = "#9aaeba";
                                    function dot(x, y) { ctx.beginPath(); ctx.arc(x, y, 1, 0, Math.PI * 2); ctx.fill(); }
                                    const columns = Math.max(1, Math.round((width - 4) / 6));
                                    const rows = Math.max(1, Math.round((height - 4) / 6));
                                    for (let i = 0; i <= columns; ++i) {
                                        dot(2 + i * (width - 4) / columns, 2);
                                        dot(2 + i * (width - 4) / columns, height - 2);
                                    }
                                    for (let i = 1; i < rows; ++i) {
                                        dot(2, 2 + i * (height - 4) / rows);
                                        dot(width - 2, 2 + i * (height - 4) / rows);
                                    }
                                }
                            }
                        }
                        Menu {
                            id: languageMenu
                            width: slotButton.width
                            y: slotButton.height
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                            onAboutToShow: customInput.textField.clear()
                            MenuItem {
                                visible: root.choices(originGroup.index).length === 0
                                height: visible ? implicitHeight : 0
                                text: qsTr("Nessuna lingua selezionabile")
                                enabled: false
                            }
                            Instantiator {
                                model: root.choices(originGroup.index)
                                delegate: MenuItem {
                                    id: choice
                                    required property string modelData
                                    text: root.languageName(modelData)
                                    contentItem: Label { text: choice.text; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                                    onTriggered: {
                                        if (root.remaining(originGroup.index) > 0 && root.choices(originGroup.index).includes(modelData))
                                            root.addRequested(modelData, originGroup.index);
                                    }
                                }
                                onObjectAdded: (index, object) => languageMenu.insertItem(index, object)
                                onObjectRemoved: (index, object) => languageMenu.removeItem(object)
                            }
                            CustomLanguageInput {
                                id: customInput
                                objectName: "customLanguage_" + originGroup.index + "_" + slotButton.index
                                visible: originGroup.index === LanguagesPanel.Linguistics
                                height: visible ? implicitHeight : 0
                                onSubmitted: name => {
                                    if (originGroup.index !== LanguagesPanel.Linguistics || root.remaining(originGroup.index) <= 0) return;
                                    root.customLearnRequested(name);
                                    languageMenu.close();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
