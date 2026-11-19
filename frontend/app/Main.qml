pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App

ApplicationWindow {
    id: window

    required property SessionController sessionController
    readonly property bool inSession: sessionController.activeName.length > 0
    readonly property bool hosting: sessionController.hostingPort > 0
    readonly property HostedCampaignPage hostedPage: navigation.currentItem as HostedCampaignPage

    width: 960
    height: 640
    minimumWidth: 360
    minimumHeight: 240
    visible: true
    title: qsTr("Golarion Helper")
    color: "#f7f8fa"
    font.pixelSize: 14
    palette.windowText: "#222831"
    palette.buttonText: "#222831"
    palette.button: "#eef1f4"
    palette.highlight: "#315b83"

    header: ToolBar {
        padding: 12
        leftPadding: 20
        rightPadding: 20

        background: Rectangle {
            color: "#ffffff"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#e1e5ea"
            }
        }

        contentItem: GridLayout {
            columns: window.sessionController.hostedCampaignOpen && window.width < 540 ? 1 : 2
            columnSpacing: 16
            rowSpacing: 6

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: window.sessionController.hostedCampaignOpen ? 40 : 0
                spacing: 12

                Button {
                    Layout.minimumHeight: 34
                    leftPadding: 14
                    rightPadding: 14
                    visible: navigation.depth > 1 &&
                             (!window.inSession || (window.sessionController.hostedCampaignOpen && !window.hosting))
                    enabled: !window.sessionController.busy
                    text: qsTr("Indietro")
                    onClicked: {
                        window.sessionController.clearError()
                        if (window.sessionController.hostedCampaignOpen)
                            window.sessionController.closeHostedCampaign()
                        else
                            navigation.pop(null, StackView.Immediate)
                    }
                }


                Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    visible: window.inSession
                    text: window.sessionController.activeName
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                TextField {
                    id: displayNameField
                    Layout.preferredWidth: 180
                    visible: !window.inSession
                    placeholderText: qsTr("Nome")
                    Accessible.name: qsTr("Nome visualizzato")
                    horizontalAlignment: TextInput.AlignRight
                    selectByMouse: true
                    palette.base: "#ffffff"
                    palette.text: window.palette.windowText
                }

                Label {
                    visible: window.sessionController.hostedCampaignOpen
                    text: qsTr("Codice %1").arg(window.sessionController.inviteCode)
                    font.weight: Font.DemiBold
                    Accessible.name: qsTr("Codice d’invito %1").arg(window.sessionController.inviteCode)
                }

                Button {
                    visible: window.sessionController.hostedCampaignOpen
                    implicitWidth: Math.max(60, implicitContentWidth + leftPadding + rightPadding)
                    text: qsTr("Copia")
                    Accessible.name: qsTr("Copia codice d’invito")
                    onClicked: window.sessionController.copyInvitationText(window.sessionController.inviteCode)
                }

                Button {
                    visible: window.inSession
                    enabled: !window.sessionController.busy &&
                             (!window.sessionController.hostedCampaignOpen || window.hosting ||
                              (window.hostedPage !== null && window.hostedPage.portValid))
                    text: {
                        if (!window.sessionController.hostedCampaignOpen)
                            return qsTr("Disconnetti")
                        if (window.hosting)
                            return qsTr("Ferma hosting")
                        return window.sessionController.busy ? qsTr("Avvio in corso…") : qsTr("Avvia hosting")
                    }
                    onClicked: {
                        if (window.hosting)
                            window.sessionController.stopHosting()
                        else if (window.hostedPage !== null)
                            window.hostedPage.startHosting()
                        else
                            window.sessionController.disconnectSession()
                    }
                }
            }
        }
    }

    StackView {
        id: navigation

        anchors.fill: parent
        initialItem: ScrollView {
            id: home

            padding: 20
            contentWidth: availableWidth

            ColumnLayout {
                width: Math.min(320, home.availableWidth)
                spacing: 12

                HomeTile {
                    text: qsTr("Sessioni")
                    onClicked: navigation.push(sessionsPage, StackView.Immediate)
                }

                HomeTile {
                    text: qsTr("Schede")
                    enabled: false
                    palette.button: "#f1f3f5"
                    palette.buttonText: "#626d79"
                }
            }
        }
    }

    Component {
        id: sessionsPage

        CampaignsPage {
            controller: window.sessionController
            localName: displayNameField.text
        }
    }

    Settings {
        category: "Profile"
        property alias displayName: displayNameField.text
    }

    Connections {
        target: window.sessionController

        function onSessionOpened() {
            const page = window.sessionController.hostedCampaignOpen ? hostedSessionPage : activeSessionPage
            navigation.push(page, StackView.Immediate)
        }

        function onSessionClosed() {
            navigation.pop(navigation.get(navigation.depth - 2), StackView.Immediate)
        }
    }

    Component {
        id: hostedSessionPage

        HostedCampaignPage {
            controller: window.sessionController
            localName: displayNameField.text
        }
    }

    Component {
        id: activeSessionPage

        ScrollView {
            id: joinedPage

            padding: 20
            contentWidth: availableWidth

            GridLayout {
                width: joinedPage.availableWidth
                columns: width < 540 ? 1 : 2
                columnSpacing: 24
                rowSpacing: 16

                Item {
                    Layout.fillWidth: true
                    implicitHeight: clientError.implicitHeight
                    visible: joinedPage.availableWidth >= 540 || clientError.visible

                    Label {
                        id: clientError
                        width: parent.width
                        visible: text.length > 0
                        text: window.sessionController.errorMessage
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }
                }

                ParticipantsPanel {
                    Layout.preferredWidth: 260
                    Layout.fillWidth: joinedPage.availableWidth < 540
                    Layout.alignment: Qt.AlignTop
                    participants: window.sessionController.participants
                }
            }
        }
    }

    component HomeTile: Button {
        id: tile

        Layout.fillWidth: true
        Layout.preferredHeight: 112
        font.pixelSize: 22
        font.weight: Font.DemiBold
        palette.button: "#ffffff"

        background: Rectangle {
            radius: 8
            color: tile.down ? "#e7edf4" : tile.palette.button
            border.color: tile.visualFocus || tile.hovered ? tile.palette.highlight : "#e1e5ea"
            border.width: tile.visualFocus ? 2 : 1
        }
    }
}
