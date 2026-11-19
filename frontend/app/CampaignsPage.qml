pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App

ScrollView {
    id: page

    enum Form { NoForm, Joining, Creating }

    required property SessionController controller
    required property string localName
    property int form: CampaignsPage.NoForm
    property int campaignIndex: -1

    padding: 20
    contentWidth: availableWidth

    function openJoinForm(index: int, address: string, code: string): void {
        controller.clearError()
        campaignIndex = index
        addressField.text = address
        codeField.text = code
        form = CampaignsPage.Joining
        addressField.forceActiveFocus()
    }

    function submitJoinForm(): void {
        if (enterButton.enabled)
            controller.enter(addressField.text, codeField.text, campaignIndex, localName)
    }

    function openCreateForm(): void {
        controller.clearError()
        nameField.clear()
        form = CampaignsPage.Creating
        nameField.forceActiveFocus()
    }

    function saveCampaign(): void {
        if (saveButton.enabled && controller.createHostedCampaign(nameField.text))
            closeForm()
    }

    function closeForm(): void {
        form = CampaignsPage.NoForm
        controller.clearError()
    }

    function confirmRemoval(campaign: var, hosted: bool): void {
        controller.clearError()
        removalDialog.hosted = hosted
        removalDialog.targetId = campaign.id
        removalDialog.campaignName = campaign.name
        removalDialog.code = campaign.code
        removalDialog.address = campaign.address ?? ""
        confirmationField.clear()
        removalDialog.open()
    }

    Dialog {
        id: removalDialog
        objectName: "campaignRemovalDialog"

        property bool hosted: false
        property string targetId: ""
        property string campaignName: ""
        property string code: ""
        property string address: ""

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: parent ? Math.min(440, parent.width - 24) : 440
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        title: hosted ? qsTr("Elimina campagna") : qsTr("Rimuovi dalla lista")
        onOpened: cancelRemoval.forceActiveFocus()

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: removalDialog.campaignName
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                visible: removalDialog.hosted ? removalDialog.code.length > 0 : removalDialog.address.length > 0
                text: removalDialog.hosted
                      ? qsTr("Codice d’invito %1").arg(removalDialog.code)
                      : removalDialog.address
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                text: removalDialog.hosted
                      ? qsTr("La cartella della campagna, con identità, permessi e credenziali dei partecipanti, verrà spostata nel Cestino di sistema.")
                      : qsTr("Verrà rimossa la credenziale salvata su questo PC. Se rientri con IP e codice, l’host ti riconoscerà come nuovo partecipante.")
                wrapMode: Text.WordWrap
            }

            TextField {
                id: confirmationField
                objectName: "campaignNameConfirmation"
                Layout.fillWidth: true
                visible: removalDialog.hosted
                placeholderText: qsTr("Digita il nome della campagna")
                Accessible.name: placeholderText
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.controller.errorMessage
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                color: "#a12d32"
            }
        }

        footer: Frame {
            contentItem: RowLayout {
                Button {
                    id: cancelRemoval
                    objectName: "cancelCampaignRemoval"
                    text: qsTr("Annulla")
                    onClicked: removalDialog.reject()
                }

                Item { Layout.fillWidth: true }

                Button {
                    objectName: "confirmCampaignRemoval"
                    text: removalDialog.hosted ? qsTr("Sposta nel Cestino") : qsTr("Rimuovi")
                    palette.buttonText: "#a12d32"
                    enabled: !page.controller.busy && (!removalDialog.hosted
                             || confirmationField.text === removalDialog.campaignName)
                    onClicked: {
                        const removed = removalDialog.hosted
                            ? page.controller.trashHostedCampaign(removalDialog.targetId, confirmationField.text, removalDialog.code)
                            : page.controller.forgetJoinedCampaign(removalDialog.targetId)
                        if (removed) {
                            page.closeForm()
                            removalDialog.accept()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: page.controller
        function onSessionOpened() {
            page.form = CampaignsPage.NoForm
        }
    }

    ColumnLayout {
        width: Math.min(560, page.availableWidth)
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: qsTr("Campagne")
                font.pixelSize: 24
                font.weight: Font.DemiBold
            }

            Button {
                id: addButton
                text: qsTr("+ Aggiungi")
                enabled: !page.controller.busy
                onClicked: addMenu.open()

                Menu {
                    id: addMenu
                    x: addButton.width - width
                    y: addButton.height

                    MenuItem {
                        text: qsTr("Crea campagna")
                        onTriggered: page.openCreateForm()
                    }

                    MenuItem {
                        text: qsTr("Unisciti con IP e codice")
                        onTriggered: page.openJoinForm(-1, "", "")
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            visible: page.form === CampaignsPage.Creating
            padding: 16

            contentItem: ColumnLayout {
                spacing: 8

                Label { text: qsTr("Nome della campagna") }

                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    Accessible.name: qsTr("Nome della campagna")
                    selectByMouse: true
                    onAccepted: page.saveCampaign()
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: qsTr("Annulla")
                        onClicked: page.closeForm()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        id: saveButton
                        text: qsTr("Salva")
                        enabled: nameField.text.trim().length > 0
                        onClicked: page.saveCampaign()
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            visible: page.form === CampaignsPage.Joining
            padding: 16

            contentItem: ColumnLayout {
                spacing: 8

                Label { text: qsTr("Indirizzo IP") }

                TextField {
                    id: addressField
                    Layout.fillWidth: true
                    enabled: !page.controller.busy
                    placeholderText: "192.168.1.10:42500"
                    Accessible.name: qsTr("Indirizzo IP")
                    selectByMouse: true
                    onAccepted: codeField.forceActiveFocus()
                }

                Label { text: qsTr("Codice sessione") }

                TextField {
                    id: codeField
                    Layout.fillWidth: true
                    enabled: !page.controller.busy
                    Accessible.name: qsTr("Codice sessione")
                    selectByMouse: true
                    onAccepted: page.submitJoinForm()
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: qsTr("Annulla")
                        enabled: !page.controller.busy
                        onClicked: page.closeForm()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        id: enterButton
                        enabled: !page.controller.busy && addressField.text.trim().length > 0
                                 && codeField.text.trim().length > 0
                        text: page.controller.busy ? qsTr("Connessione…")
                              : page.campaignIndex < 0 ? qsTr("Unisciti") : qsTr("Rientra")
                        onClicked: page.submitJoinForm()
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: page.controller.errorMessage
            color: "#a12d32"
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            visible: page.form === CampaignsPage.NoForm && page.controller.campaigns.length === 0
                     && page.controller.hostedCampaigns.length === 0
            text: qsTr("Non hai ancora campagne.")
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: page.controller.hostedCampaigns

            delegate: CampaignRow {
                id: hostedRow

                required property int index
                required property var modelData

                enabled: !page.controller.busy
                text: hostedRow.modelData.name
                status: qsTr("Da avviare")
                removalText: qsTr("Elimina")
                removalLabel: qsTr("Elimina campagna %1").arg(text)
                removalButtonName: "trashHostedCampaignButton"
                onOpenRequested: page.controller.openHostedCampaign(hostedRow.index)
                onRemovalRequested: page.confirmRemoval(hostedRow.modelData, true)
            }
        }

        Repeater {
            model: page.controller.campaigns

            delegate: CampaignRow {
                id: joinedRow

                required property int index
                required property var modelData

                enabled: !page.controller.busy
                text: joinedRow.modelData.name
                removalText: qsTr("Rimuovi")
                removalLabel: qsTr("Rimuovi %1 dalla lista").arg(text)
                removalButtonName: "forgetJoinedCampaignButton"
                onOpenRequested: page.openJoinForm(joinedRow.index, joinedRow.modelData.address, joinedRow.modelData.code)
                onRemovalRequested: page.confirmRemoval(joinedRow.modelData, false)
            }
        }
    }

    component CampaignRow: RowLayout {
        id: item

        required property string text
        property string status
        required property string removalText
        required property string removalLabel
        required property string removalButtonName
        signal openRequested()
        signal removalRequested()

        Layout.fillWidth: true

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: item.text
            onClicked: item.openRequested()

            contentItem: RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: item.text
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                }

                Label {
                    visible: item.status.length > 0
                    text: item.status
                    color: "#626d79"
                }
            }
        }

        Button {
            objectName: item.removalButtonName
            text: item.removalText
            Accessible.name: item.removalLabel
            onClicked: item.removalRequested()
        }
    }
}
