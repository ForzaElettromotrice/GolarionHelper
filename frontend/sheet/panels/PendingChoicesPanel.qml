pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum ChoiceType { Race, Alignment, RacialOption, ClassOption, FeatSlot, ClassHitDie,
        FavoredClass, FavoredClassBonus, SkillRanks, Language, PreparedSpell, FlexiblePreparation, Repertoire }
    required property var pendingChoices
    signal openChoiceRequested(int type, var choiceReference)
    spacing: 12

    Label { text: qsTr("Scelte aperte"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    Label {
        visible: !(root.pendingChoices?.length)
        text: qsTr("Nessuna scelta aperta.")
        color: "#536976"
    }
    Repeater {
        model: [
            {title: qsTr("Bloccano il prossimo livello"), blocking: true},
            {title: qsTr("Non bloccanti"), blocking: false}
        ]
        Column {
            id: section
            required property var modelData
            readonly property var choices: (root.pendingChoices ?? []).filter(choice => choice.blocksAdvancement === modelData.blocking)
            objectName: modelData.blocking ? "blockingChoices" : "optionalChoices"
            width: root.width
            visible: choices.length > 0
            spacing: 5
            Label { text: section.modelData.title; font.bold: true; color: "#253947" }
            Repeater {
                model: section.choices
                ItemDelegate {
                    id: choiceRow
                    required property var modelData
                    required property int index
                    objectName: section.objectName + "_" + index
                    width: section.width
                    Accessible.name: modelData.description
                    onClicked: root.openChoiceRequested(modelData.type, modelData.choiceReference)
                    background: Rectangle {
                        color: choiceRow.hovered ? "#e9eef1" : "#fafbfc"
                        border.color: choiceRow.visualFocus ? "#52768e" : "#d2dbe1"
                        radius: 5
                    }
                    contentItem: RowLayout {
                        Label {
                            Layout.fillWidth: true
                            text: choiceRow.modelData.description
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            color: "#253947"
                        }
                        Label {
                            visible: choiceRow.modelData.remaining > 1
                            text: "×" + choiceRow.modelData.remaining
                            color: "#536976"
                        }
                        Label { text: "›"; color: "#52768e" }
                    }
                }
            }
        }
    }
}
