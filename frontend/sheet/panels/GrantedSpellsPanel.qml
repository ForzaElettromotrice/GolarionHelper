pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum Usage { Limited, AtWill, Constant, SharedPool }
    required property var grantedSpells
    property var spellNames: ({})
    signal castGrantedSpellRequested(string grantId)
    spacing: 8

    function availability(grant) {
        switch (grant.usage) {
        case GrantedSpellsPanel.Limited: return qsTr("%1/%2 utilizzi").arg(grant.remaining).arg(grant.capacity);
        case GrantedSpellsPanel.AtWill: return qsTr("A volontà");
        case GrantedSpellsPanel.Constant: return qsTr("Permanente");
        case GrantedSpellsPanel.SharedPool:
            return (grant.remaining === 1 ? qsTr("%1 carica") : qsTr("%1 cariche")).arg(grant.remaining);
        default: return "";
        }
    }
    function canCast(grant) {
        if (grant.usage === GrantedSpellsPanel.AtWill) return true;
        return (grant.usage === GrantedSpellsPanel.Limited || grant.usage === GrantedSpellsPanel.SharedPool)
            && grant.cost > 0 && grant.remaining != null && grant.remaining >= grant.cost;
    }

    Label { text: qsTr("Incantesimi concessi"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    Label {
        visible: !(root.grantedSpells?.length)
        text: qsTr("Nessun incantesimo concesso.")
        color: "#536976"
    }
    Repeater {
        model: root.grantedSpells ?? []
        Rectangle {
            id: spellCard
            required property var modelData
            objectName: "grantedSpell_" + modelData.id
            width: root.width
            implicitHeight: spellRow.implicitHeight + 16
            color: "#fafbfc"
            border.color: "#d2dbe1"
            radius: 5
            RowLayout {
                id: spellRow
                x: 10
                y: 8
                width: parent.width - 20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        id: spellName
                        Layout.fillWidth: true
                        text: root.spellNames[spellCard.modelData.spellId] ?? spellCard.modelData.spellId
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.bold: true
                        color: "#253947"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: spellCard.modelData.source
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: "#536976"
                    }
                }
                ColumnLayout {
                    spacing: 2
                    Label {
                        objectName: "grantedSpellAvailability"
                        text: root.availability(spellCard.modelData)
                        color: "#365f75"
                    }
                    Label {
                        visible: spellCard.modelData.usage === GrantedSpellsPanel.SharedPool
                        text: qsTr("%1 per lancio").arg(spellCard.modelData.cost)
                        color: "#536976"
                    }
                }
                Button {
                    objectName: "castGrantedSpellButton"
                    visible: spellCard.modelData.usage !== GrantedSpellsPanel.Constant
                    enabled: root.canCast(spellCard.modelData)
                    text: qsTr("Lancia")
                    Accessible.name: qsTr("Lancia %1").arg(spellName.text)
                    onClicked: root.castGrantedSpellRequested(spellCard.modelData.id)
                }
            }
        }
    }
}
