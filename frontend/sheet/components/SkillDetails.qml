import QtQuick
import "ModifierText.js" as ModifierText

InfoDetails {
    required property var skill
    function signed(value) { return value === undefined ? "—" : (value >= 0 ? "+" : "") + value; }

    rows: {
        const lines = [{kind: InfoDetails.Heading, text: qsTr("Gradi")}];
        lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(skill.ranks) + qsTr(" Assegnati da te")});
        for (const grant of skill.rankGrants ?? [])
            lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(grant.ranks) + " " + grant.source});
        lines.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + (skill.effectiveRanks ?? skill.ranks)});
        if ((skill.rankGrants ?? []).length)
            lines.push({kind: InfoDetails.Note, text: qsTr("Si usa il valore maggiore, non la somma.")});
        lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Heading, text: qsTr("Punteggio")});
        if (!skill.usable)
            lines.push({kind: InfoDetails.Note, text: skill.trainedOnly ? qsTr("Richiede addestramento.") : qsTr("Abilità non utilizzabile.")});
        const abilityNames = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        const options = skill.abilityOptions ?? [];
        for (const option of options) {
            const ability = abilityNames[option.abilityType] ?? qsTr("Caratteristica non specificata");
            if (options.length > 1)
                lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Group, text: ability + (option.source ? " — " + option.source : "")});
            lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(skill.effectiveRanks ?? skill.ranks) + qsTr(" Gradi")});
            lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(option.abilityModifier) + " " + ability});
            if (skill.classSkillBonus)
                lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(skill.classSkillBonus) + qsTr(" Abilità di classe")});
            if (skill.appliedArmorCheckPenalty)
                lines.push({kind: InfoDetails.Entry, indent: 2, text: signed(-skill.appliedArmorCheckPenalty) + qsTr(" Armatura")});
            lines.push(...ModifierText.rows(option.modifiers, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
            lines.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + signed(option.totalValue)});
        }
        lines.push({kind: InfoDetails.Spacer, text: ""}, {kind: InfoDetails.Note, text: qsTr("I bonus attivi non sono necessariamente cumulabili.")});
        return lines;
    }
}
