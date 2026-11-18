import QtQuick
import "ModifierText.js" as ModifierText
import "DamageText.js" as DamageText

InfoDetails {
    id: root
    required property var attack
    property var baseAttackBonus: null
    enum DamageTypeMode { All, Choice }
    framed: false
    padding: 0
    rows: buildRows()

    function buildRows() {
        const result = [];
        const strike = attack.strike ?? {};
        function entry(text) { result.push({kind: InfoDetails.Entry, text: text}); }
        function heading(text) {
            if (result.length) result.push({kind: InfoDetails.Spacer, text: ""});
            result.push({kind: InfoDetails.Heading, text: text});
        }
        function signed(value) { return value >= 0 ? "+" + value : String(value); }
        function modifiers(set) {
            if (set) result.push(...ModifierText.rows(set, AbilityScoresPanel.Penalty, InfoDetails.Group, InfoDetails.Entry));
        }
        if (strike.source) entry(qsTr("Fonte: ") + strike.source);
        if (attack.slotName) entry(qsTr("Slot: ") + attack.slotName);
        for (const reason of strike.failureReasons ?? []) entry(reason);
        const defenses = [qsTr("CA"), qsTr("CA di contatto"), qsTr("CA impreparato")];
        for (const option of strike.defenseOptions ?? [])
            entry(qsTr("Contro: ") + (defenses[option.defenseType] ?? "—") + (option.condition ? " · " + option.condition : ""));
        function metres(value) { return String(value * 1.5).replace(".", ",") + " m"; }
        if (strike.reach) entry(qsTr("Portata: ") + metres(strike.reach.minimumUnits.effectiveValue) + " – " + metres(strike.reach.maximumUnits.effectiveValue));
        if (strike.range) {
            entry(qsTr("Incremento di gittata: ") + metres(strike.range.incrementUnits.effectiveValue));
            entry(qsTr("Incrementi massimi: ") + strike.range.maximumIncrements.effectiveValue);
            entry(qsTr("Penalità per incremento aggiuntivo: ") + strike.range.penaltyPerAdditionalIncrement.effectiveValue);
        }
        const abilities = [qsTr("Forza"), qsTr("Destrezza"), qsTr("Costituzione"), qsTr("Intelligenza"), qsTr("Saggezza"), qsTr("Carisma")];
        function attackBreakdown(title, confirmation) {
            heading(title);
            const options = strike.attackAbilityOptions ?? [];
            for (const option of options) {
                if (options.length > 1) result.push({kind: InfoDetails.Group, text: option.source || abilities[option.abilityType] || qsTr("Alternativa")});
                if (root.baseAttackBonus?.total != null) entry(signed(root.baseAttackBonus.total) + " BAB");
                else entry(qsTr("BAB non disponibile"));
                if (option.abilityModifier != null) entry(signed(option.abilityModifier) + " " + (abilities[option.abilityType] ?? qsTr("Caratteristica")));
                if (strike.usageAttackPenalty) entry(signed(strike.usageAttackPenalty) + qsTr(" Uso dell’attacco"));
                modifiers(confirmation ? strike.criticalConfirmationModifiers : strike.attackModifiers);
                if (attack.routineAttackBonusAdjustment) entry(signed(attack.routineAttackBonusAdjustment) + qsTr(" Routine"));
                if (attack.progressionAttackBonusAdjustment) entry(signed(attack.progressionAttackBonusAdjustment) + qsTr(" Progressione"));
                const total = confirmation ? option.criticalConfirmationBonus : option.attackBonus;
                if (total != null) result.push({kind: InfoDetails.Total, text: qsTr("Totale: ") + signed(total)});
            }
        }
        attackBreakdown(qsTr("Tiro per colpire"), false);
        if ((strike.attackAbilityOptions ?? []).some(option => option.criticalConfirmationBonus != null && option.criticalConfirmationBonus !== option.attackBonus)) {
            attackBreakdown(qsTr("Conferma del critico"), true);
        }
        heading(qsTr("Danni"));
        function damageAbilities(damage) {
            const options = damage?.abilityOptions ?? [];
            for (const option of options) {
                if (options.length > 1) result.push({kind: InfoDetails.Group, text: option.source || abilities[option.abilityType] || qsTr("Alternativa")});
                if (option.abilityContribution != null) entry(signed(option.abilityContribution) + " " + (abilities[option.abilityType] ?? qsTr("Caratteristica")));
            }
        }
        damageAbilities(attack.normalDamage);
        modifiers(strike.damageModifiers);
        for (const component of strike.damageComponents ?? []) {
            entry((component.effectiveDice?.expression ?? "—") + " · " + (component.source ?? ""));
            entry(qsTr("Tipo: ") + DamageText.types(component, AttackDetails.Choice));
            for (const variant of component.conditionalDice ?? []) entry(variant.condition + ": " + variant.dice.expression);
        }
        for (const variant of strike.conditionalCriticalProfiles ?? []) {
            const profile = variant.profile;
            entry(qsTr("Critico · ") + variant.condition + ": " + profile.threatMinimum + "–20 / ×" + profile.multiplier);
        }
        if (attack.criticalDamage?.abilityOptions?.some(option => option.abilityContribution != null)) {
            heading(qsTr("Danni critici"));
            damageAbilities(attack.criticalDamage);
        }
        for (const requirement of strike.requirements ?? []) {
            if (requirement.requirement?.satisfied === false) entry(requirement.requirement.failureReason);
        }
        result.push({kind: InfoDetails.Note, text: qsTr("I valori finali includono già le modifiche della routine e della progressione.")});
        return result;
    }
}
