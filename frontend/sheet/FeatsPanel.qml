pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: root
    enum SlotOrigin { Source, Extra }
    enum FeatType { General, Combat, Critical, ItemCreation, Metamagic, Teamwork, Style, Achievement, Channeling, Grit, Panache, Performance, Racial, Story, Mythic, Monster, Stare, HeroPoint, Alignment }
    required property var feats
    property bool prominentHeading: false
    signal selectRequested(string slotId)
    signal extraRequested()
    signal removeRequested(string slotId)
    signal removeExtraSlotRequested(string slotId)
    implicitWidth: 360
    implicitHeight: content.height

    function featFor(slotId) { return (feats?.feats ?? []).find(feat => feat.slotId === slotId) ?? null; }
    function typeName(type) {
        return [qsTr("Generale"), qsTr("Combattimento"), qsTr("Critico"), qsTr("Creazione oggetto"), qsTr("Metamagia"),
            qsTr("Squadra"), qsTr("Stile"), qsTr("Impresa"), qsTr("Incanalare"), qsTr("Grinta"), qsTr("Eleganza"),
            qsTr("Esibizione"), qsTr("Razziale"), qsTr("Storia"), qsTr("Mitico"), qsTr("Mostruoso"), qsTr("Occhiata"),
            qsTr("Punto eroe"), qsTr("Allineamento")][type] ?? "—";
    }
    function restrictions(slot) {
        if (slot.origin === FeatsPanel.Extra) return qsTr("Extra · ignora i prerequisiti");
        if (slot.allowedType != null) return typeName(slot.allowedType);
        if (slot.allowedFeatIds?.length) return qsTr("Scelta da un insieme specifico");
        return qsTr("Talento a scelta");
    }
    function details(slot, feat) {
        const rows = [{kind: InfoDetails.Heading, text: qsTr("Descrizione")},
            {kind: InfoDetails.Entry, text: feat.description || qsTr("Nessuna descrizione disponibile.")}];
        if (feat.types?.length) rows.push({kind: InfoDetails.Note, text: feat.types.map(type => typeName(type)).join(" · ")});
        for (const choice of feat.choices ?? []) {
            rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: choice.prompt});
            for (const option of choice.selectedOptions ?? []) rows.push({kind: InfoDetails.Entry, text: option.name});
        }
        rows.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: qsTr("Acquisizione")},
            {kind: InfoDetails.Entry, text: slot.source}, {kind: InfoDetails.Note, text: restrictions(slot)});
        if (slot.available === false) rows.push({kind: InfoDetails.Note, text: qsTr("La fonte di questo slot non è più disponibile. La scelta è conservata.")});
        if (!feat.active) rows.push({kind: InfoDetails.Note, text: qsTr("Talento inattivo: i suoi effetti non sono applicati.")});
        return rows;
    }
    Column {
        id: content
        width: root.prominentHeading ? root.width : Math.min(root.width, root.implicitWidth)
        spacing: 6
        Row {
            width: parent.width
            Label {
                objectName: "sectionHeading"
                width: parent.width - 32
                height: root.prominentHeading ? 42 : 32
                text: qsTr("Talenti")
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
                objectName: "addExtraFeat"
                width: 32; height: root.prominentHeading ? 42 : 32
                palette.buttonText: root.prominentHeading ? "white" : "#181818"
                text: "+"; font.pixelSize: 24
                Accessible.name: qsTr("Aggiungi talento extra senza slot né prerequisiti")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Aggiungi talento extra")
                onClicked: root.extraRequested()
            }
        }
        Label {
            visible: !(root.feats?.slots?.length)
            width: parent.width
            text: qsTr("Nessuno slot talento disponibile.")
            wrapMode: Text.Wrap
            color: "#777777"
        }
        Repeater {
            model: root.feats?.slots ?? []
            Column {
                id: slot
                required property var modelData
                readonly property var feat: root.featFor(modelData.id)
                width: content.width
                spacing: 4
                Rectangle {
                    visible: !!slot.feat
                    width: parent.width
                    implicitHeight: Math.max(56, labels.height + 20)
                    color: "#f2f5f7"
                    border.color: "#c6cfd5"
                    radius: 4
                    Column {
                        id: labels
                        x: 10; y: 10
                        width: parent.width - 80
                        spacing: 3
                        Label {
                            objectName: "featName_" + slot.modelData.id
                            width: parent.width
                            text: slot.feat?.name ?? slot.modelData.source
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.pixelSize: 15
                            font.bold: !!slot.feat
                            color: slot.feat && !slot.feat.active ? "#777777" : "#253947"
                        }
                        Label {
                            width: parent.width
                            text: slot.feat ? slot.modelData.source : root.restrictions(slot.modelData)
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                            color: "#666666"
                        }
                        Label {
                            objectName: "featStatus_" + slot.modelData.id
                            visible: !!slot.feat && (!slot.feat.active || slot.modelData.available === false)
                            width: parent.width
                            text: slot.modelData.available === false ? qsTr("Inattivo · slot non disponibile") : qsTr("Inattivo")
                            wrapMode: Text.Wrap
                            color: "#a34a34"
                            font.pixelSize: 12
                        }
                    }
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 5
                        anchors.verticalCenter: parent.verticalCenter
                        InfoButton {
                            id: info
                            objectName: "featInfo_" + slot.modelData.id
                            visible: !!slot.feat
                            height: 32
                            Accessible.name: qsTr("Informazioni sul talento ") + (slot.feat?.name ?? "")
                        }
                        ToolButton {
                            id: remove
                            objectName: "removeFeat_" + slot.modelData.id
                            visible: !!slot.feat || slot.modelData.origin === FeatsPanel.Extra
                            width: 28; height: 32
                            Accessible.name: slot.modelData.origin === FeatsPanel.Extra ? qsTr("Rimuovi slot extra e talento") : qsTr("Rimuovi talento mantenendo lo slot")
                            background: Item {}
                            contentItem: Label {
                                text: "×"; font.pixelSize: 22
                                color: remove.hovered ? "#b33434" : "#777777"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: slot.modelData.origin === FeatsPanel.Extra
                                ? root.removeExtraSlotRequested(slot.modelData.id) : root.removeRequested(slot.modelData.id)
                        }
                    }
                }
                Button {
                    id: select
                    objectName: "selectFeat_" + slot.modelData.id
                    visible: !slot.feat
                    enabled: slot.modelData.available !== false && !slot.modelData.featId
                    width: parent.width
                    padding: 12
                    rightPadding: slot.modelData.origin === FeatsPanel.Extra ? 40 : 12
                    Accessible.name: qsTr("Scegli talento: ") + slot.modelData.source
                    onClicked: root.selectRequested(slot.modelData.id)
                    contentItem: Row {
                        spacing: 8
                        Column {
                            id: emptyLabels
                            width: parent.width - 40
                            spacing: 3
                            Label {
                                objectName: "emptyFeatSource_" + slot.modelData.id
                                width: parent.width
                                text: slot.modelData.source
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 15
                                color: "#253947"
                            }
                            Label {
                                width: parent.width
                                text: root.restrictions(slot.modelData)
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.pixelSize: 12
                                color: "#666666"
                            }
                        }
                        Label {
                            width: 32
                            height: emptyLabels.height
                            text: "+"; font.pixelSize: 26
                            color: select.enabled ? "#68808f" : "#999999"
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                    background: Rectangle {
                        color: select.hovered ? "#edf3f7" : "#f8fafb"
                        border.color: select.visualFocus ? "#52768e" : "#c6cfd5"
                        radius: 4
                    }
                    ToolButton {
                        objectName: "removeEmptyExtraFeat_" + slot.modelData.id
                        visible: slot.modelData.origin === FeatsPanel.Extra
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28; height: 32
                        text: "×"
                        Accessible.name: qsTr("Rimuovi slot extra vuoto")
                        onClicked: root.removeExtraSlotRequested(slot.modelData.id)
                    }
                }
                InfoDetails {
                    objectName: "featDetails_" + slot.modelData.id
                    width: parent.width
                    visible: !!slot.feat && info.checked
                    rows: slot.feat ? root.details(slot.modelData, slot.feat) : []
                }
            }
        }
    }
}
