pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.App

Dialog {
    id: dialog
    objectName: "participantPermissionsDialog"

    required property SessionController controller
    required property bool canEditDelegation
    property string targetUserId: ""
    property string displayName: ""
    property int loadedMask: -1
    property int draftMask: 0
    property bool loading: false
    property bool saving: false
    property string requestError: ""

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: parent ? Math.min(420, parent.width - 24) : 420
    height: parent ? Math.min(implicitHeight, parent.height - 24) : implicitHeight
    modal: true
    focus: true
    closePolicy: saving ? Popup.NoAutoClose : Popup.CloseOnEscape
    title: qsTr("Permessi")
    onOpened: cancelButton.forceActiveFocus()
    onAboutToHide: controller.cancelParticipantPermissionRead()

    function openFor(userId: string, name: string): void {
        targetUserId = userId
        displayName = name
        loadedMask = -1
        draftMask = 0
        saving = false
        open()
        load()
    }

    function load(): void {
        requestError = ""
        loading = true
        controller.readParticipantPermissions(targetUserId)
    }

    Connections {
        target: dialog.controller
        enabled: dialog.visible

        function onParticipantPermissionsLoaded(mask: int): void {
            dialog.draftMask = mask
            dialog.loadedMask = mask
            dialog.loading = false
        }

        function onParticipantPermissionsSaved(): void {
            dialog.saving = false
            dialog.accept()
        }

        function onParticipantPermissionsFailed(message: string): void {
            dialog.requestError = message
            dialog.loading = false
            dialog.saving = false
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: dialog.displayName
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.weight: Font.DemiBold
        }

        Label {
            visible: dialog.loading
            text: qsTr("Caricamento dei permessi…")
        }

        ScrollView {
            id: permissionScroll
            visible: dialog.loadedMask >= 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: options.implicitHeight
            clip: true
            contentWidth: availableWidth
            contentHeight: options.implicitHeight

            ColumnLayout {
                id: options
                width: permissionScroll.availableWidth
                spacing: 2

                Repeater {
                    model: [
                        { flag: 1, label: qsTr("Permission1") },
                        { flag: 2, label: qsTr("Permission2") },
                        { flag: 4, label: qsTr("Permission3") },
                        { flag: 8, label: qsTr("Permission4") },
                        { flag: 16, label: qsTr("Permission5") },
                        { flag: 32, label: qsTr("Gestire i permessi") }
                    ]
                    delegate: CheckBox {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.label
                        enabled: !dialog.saving && (modelData.flag !== 32 || dialog.canEditDelegation)
                        checked: (dialog.draftMask & modelData.flag) !== 0
                        onToggled: {
                            if (checked)
                                dialog.draftMask |= modelData.flag
                            else
                                dialog.draftMask &= ~modelData.flag
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: dialog.requestError
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: "#a12d32"
        }
    }

    footer: Frame {
        contentItem: RowLayout {
            Item { Layout.fillWidth: true }

            Button {
                id: cancelButton
                text: qsTr("Annulla")
                enabled: !dialog.saving
                onClicked: dialog.reject()
            }

            Button {
                visible: dialog.loadedMask < 0 && !dialog.loading && dialog.requestError.length > 0
                text: qsTr("Riprova")
                onClicked: dialog.load()
            }

            Button {
                visible: dialog.loadedMask >= 0
                enabled: dialog.loadedMask !== dialog.draftMask && !dialog.saving
                text: dialog.saving ? qsTr("Salvataggio…") : qsTr("Salva")
                onClicked: {
                    dialog.requestError = ""
                    dialog.saving = true
                    const grant = dialog.draftMask & ~dialog.loadedMask
                    const revoke = dialog.loadedMask & ~dialog.draftMask
                    dialog.controller.updateParticipantPermissions(dialog.targetUserId, grant, revoke)
                }
            }
        }
    }
}
