pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Dialog {
    id: root
    required property var catalog
    property string acquisitionLabel: ""
    property string slotId: ""
    property bool extra: false
    signal addRequested(string slotId, string featId)
    signal extraRequested(string featId)
    parent: Overlay.overlay
    modal: true
    title: qsTr("Scegli un talento")
    width: Math.min(520, (parent?.width ?? 544) - 24)
    x: ((parent?.width ?? 544) - width) / 2
    y: Math.max(12, ((parent?.height ?? 600) - height) / 2)
    readonly property var filtered: (catalog ?? []).filter(entry => entry.name.toLocaleLowerCase().includes(search.text.trim().toLocaleLowerCase()))
        .slice().sort((a, b) => a.name.localeCompare(b.name, "it") || a.id.localeCompare(b.id))
    onAboutToShow: search.clear()
    onOpened: search.forceActiveFocus()
    footer: DialogButtonBox {
        Button {
            text: qsTr("Annulla")
            onClicked: root.reject()
        }
    }
    contentItem: Column {
        spacing: 10
        Label {
            width: parent.width
            visible: text.length > 0
            text: root.acquisitionLabel
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: "#555555"
        }
        TextField {
            id: search
            objectName: "featCatalogSearch"
            width: parent.width
            placeholderText: qsTr("Cerca un talento…")
            Accessible.name: placeholderText
            selectByMouse: true
            ContextMenu.menu: null
        }
        ListView {
            id: list
            objectName: "featCatalogList"
            width: parent.width
            height: Math.min(360, (root.parent?.height ?? 600) * 0.5)
            clip: true
            model: root.filtered
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: candidate
                required property var modelData
                objectName: "catalogFeat_" + modelData.id
                width: list.width
                enabled: root.extra || root.slotId.length > 0
                contentItem: Label {
                    text: candidate.modelData.name
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    color: "#253947"
                    font.pixelSize: 15
                }
                onClicked: {
                    if (root.extra) root.extraRequested(modelData.id);
                    else root.addRequested(root.slotId, modelData.id);
                    root.close();
                }
            }
        }
        Label {
            width: parent.width
            visible: root.filtered.length === 0
            text: root.catalog?.length ? qsTr("Nessun talento corrispondente.") : qsTr("Catalogo non disponibile.")
            color: "#777777"
            wrapMode: Text.Wrap
        }
    }
}
