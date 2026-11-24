pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property Component languagesPage
    required property Component featsPage
    required property var reminders
    property var tabForChoice: null
    signal openChoiceRequested(int type)
    implicitHeight: featsLayout.implicitHeight
    GridLayout {
        id: featsLayout
        width: columns * columnWidth + (columns - 1) * columnSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        readonly property real columnWidth: Math.floor((parent.width - (columns - 1) * columnSpacing) / columns)
        columns: parent.width >= 1040 ? 3 : parent.width >= 820 ? 2 : 1
        columnSpacing: 24
        rowSpacing: 16

        Loader {
            objectName: "sheetFeatLanguages"
            Layout.row: 0
            Layout.column: 0
            Layout.rowSpan: featsLayout.columns === 2 ? 2 : 1
            Layout.fillWidth: true
            Layout.minimumWidth: featsLayout.columnWidth
            Layout.preferredWidth: featsLayout.columnWidth
            Layout.maximumWidth: featsLayout.columnWidth
            Layout.alignment: Qt.AlignTop
            sourceComponent: root.languagesPage
            onLoaded: item.prominentHeading = true
        }
        RemindersPanel {
            objectName: "sheetFeatReminders"
            prominentHeading: true
            Layout.row: featsLayout.columns === 1 ? 1 : 0
            Layout.column: featsLayout.columns === 1 ? 0 : 1
            Layout.fillWidth: true
            Layout.minimumWidth: featsLayout.columnWidth
            Layout.preferredWidth: featsLayout.columnWidth
            Layout.maximumWidth: featsLayout.columnWidth
            Layout.alignment: Qt.AlignTop
            reminders: root.reminders
            tabForChoice: root.tabForChoice
            onOpenChoiceRequested: type => root.openChoiceRequested(type)
        }
        Loader {
            objectName: "sheetFeatsPreview"
            Layout.row: 3 - featsLayout.columns
            Layout.column: featsLayout.columns - 1
            Layout.fillWidth: true
            Layout.minimumWidth: featsLayout.columnWidth
            Layout.preferredWidth: featsLayout.columnWidth
            Layout.maximumWidth: featsLayout.columnWidth
            Layout.alignment: Qt.AlignTop
            sourceComponent: root.featsPage
            onLoaded: item.prominentHeading = true
        }
    }
}
