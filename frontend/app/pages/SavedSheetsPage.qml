pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore
import Golarion.App

ScrollView {
    id: page
    required property SheetController controller

    padding: 20
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    StackView.onActivated: controller.refreshSavedSheets()

    Dialog {
        id: createDialog
        title: qsTr("Nuova scheda")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { sheetName.forceActiveFocus(); sheetName.selectAll() }
        onAccepted: page.controller.createSavedSheet(sheetName.text)
        TextField {
            id: sheetName
            placeholderText: qsTr("Nome della scheda")
            onAccepted: if (text.trim()) createDialog.accept()
        }
    }

    FileDialog {
        id: exportDialog
        property url source
        title: qsTr("Esporta scheda")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Schede JSON (*.json)")]
        onAccepted: page.controller.exportSavedSheet(source, selectedFile)
    }

    FileDialog {
        id: importDialog
        title: qsTr("Importa scheda")
        nameFilters: [qsTr("Schede JSON (*.json)"), qsTr("Tutti i file (*)")]
        onAccepted: page.controller.importSavedSheet(selectedFile)
    }

    ColumnLayout {
        width: Math.min(760, page.availableWidth)
        spacing: 12

        Label {
            text: qsTr("Schede")
            font.pixelSize: 24
            font.weight: Font.DemiBold
        }

        RowLayout {
            enabled: !page.controller.opening
            Button {
                text: qsTr("Nuova scheda")
                onClicked: {
                    sheetName.text = qsTr("Nuova scheda")
                    createDialog.open()
                }
            }
            Button {
                text: qsTr("Importa scheda")
                onClicked: importDialog.open()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: page.controller.opening ? qsTr("Caricamento scheda…") : page.controller.errorMessage
            wrapMode: Text.Wrap
        }

        Label {
            visible: page.controller.savedSheets.length === 0
            text: qsTr("Nessuna scheda salvata.")
            color: "#56616b"
        }

        Repeater {
            model: page.controller.savedSheets

            RowLayout {
                id: entry
                required property var modelData
                Layout.fillWidth: true
                enabled: !page.controller.opening

                ItemDelegate {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    enabled: entry.modelData.available
                    onClicked: page.controller.openSavedSheet(entry.modelData.url)
                    contentItem: ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: entry.modelData.name
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                            font.weight: Font.DemiBold
                        }
                        Label {
                            Layout.fillWidth: true
                            text: entry.modelData.available ? entry.modelData.path
                                : qsTr("Non trovata · %1").arg(entry.modelData.path)
                            textFormat: Text.PlainText
                            elide: Text.ElideMiddle
                            color: "#56616b"
                        }
                    }
                }
                Button {
                    text: qsTr("Esporta")
                    enabled: entry.modelData.available
                    onClicked: {
                        exportDialog.source = entry.modelData.url
                        const documents = StandardPaths.writableLocation(StandardPaths.DocumentsLocation).toString()
                        if (documents)
                            exportDialog.selectedFile = documents.replace(/\/$/, "") + "/" + encodeURIComponent(entry.modelData.name + ".json")
                        exportDialog.open()
                    }
                }
                Button {
                    text: qsTr("Rimuovi dall’elenco")
                    onClicked: page.controller.forgetSavedSheet(entry.modelData.url)
                }
            }
        }
    }
}
