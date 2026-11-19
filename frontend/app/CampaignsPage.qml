pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App

ScrollView {
    id: page

    enum Form { NoForm, Joining, Creating }

    required property SessionController controller
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
            controller.enter(addressField.text, codeField.text, campaignIndex)
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
                required property string modelData

                enabled: !page.controller.busy
                text: hostedRow.modelData
                status: qsTr("Da avviare")
                onClicked: page.controller.openHostedCampaign(hostedRow.index)
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
                onClicked: page.openJoinForm(joinedRow.index, joinedRow.modelData.address, joinedRow.modelData.code)
            }
        }
    }

    component CampaignRow: ItemDelegate {
        id: item

        property string status
        Layout.fillWidth: true

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
}
