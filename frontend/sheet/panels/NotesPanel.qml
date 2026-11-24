pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Column {
    id: root
    required property string notes
    property real editorHeight: 320
    signal notesChangeRequested(string notes)
    property string previousNotes: ""
    spacing: 8

    onNotesChanged: {
        if (editor && editor.text === previousNotes) editor.text = notes;
        previousNotes = notes;
    }
    Component.onCompleted: {
        previousNotes = notes;
        editor.text = notes;
    }

    Label { text: qsTr("Note"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    ScrollView {
        id: notesScroll
        objectName: "sheetNotesScroll"
        width: parent.width
        height: root.editorHeight
        clip: true
        Binding {
            target: notesScroll.contentItem
            property: "boundsBehavior"
            value: Flickable.StopAtBounds
        }
        TextArea {
            id: editor
            objectName: "sheetNotesEditor"
            placeholderText: qsTr("Scrivi qui le tue note…")
            Accessible.name: qsTr("Note della scheda")
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.PlainText
            selectByMouse: true
            ContextMenu.menu: null
            function handleEnter(event) {
                if (event.modifiers & Qt.ShiftModifier) {
                    const start = selectionStart;
                    if (start !== selectionEnd) remove(start, selectionEnd);
                    insert(start, "\n");
                    cursorPosition = start + 1;
                } else {
                    const submittedNotes = text;
                    focus = false;
                    if (submittedNotes !== root.notes) root.notesChangeRequested(submittedNotes);
                }
                event.accepted = true;
            }
            Keys.onReturnPressed: event => handleEnter(event)
            Keys.onEnterPressed: event => handleEnter(event)
        }
    }
    Label {
        text: qsTr("Invio per salvare · Maiusc+Invio per andare a capo")
        color: "#536976"
        width: parent.width
        wrapMode: Text.Wrap
    }
}
