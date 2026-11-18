import QtQuick
import QtQuick.Controls

Row {
    id: root
    enum BenefitType { HitPoint, SkillRank, Other }
    required property var types
    spacing: 3
    Label { visible: root.types.includes(FavoredBonusIcons.HitPoint); text: "♥"; font.pixelSize: 17; color: "#253947"; height: 22; verticalAlignment: Text.AlignVCenter }
    Image { objectName: "skillRankBookIcon"; visible: root.types.includes(FavoredBonusIcons.SkillRank); source: "open-book.svg"; width: 22; height: 22; sourceSize.width: 44; sourceSize.height: 44 }
    Label { visible: root.types.includes(FavoredBonusIcons.Other); text: "✦"; font.pixelSize: 17; color: "#253947"; height: 22; verticalAlignment: Text.AlignVCenter }
    Label { visible: !root.types.length; text: "+"; font.pixelSize: 16; height: 22; verticalAlignment: Text.AlignVCenter }
}
