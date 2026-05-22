package org.golarion.model.api;

import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.attack.AttackType;

import java.util.List;
import java.util.UUID;

public record AttackEntryData(
        UUID id,
        String name,
        AttackType attackType,
        AbilityType attackAbility,
        AbilityType damageAbility,
        List<AttackDamageData> damages,
        List<AttackDamageData> criticalDamages,
        String totalCriticalDamage,
        int criticalThreatRange,
        int criticalMultiplier,
        List<ModifierData> attackModifiers,
        List<ModifierData> damageModifiers,
        List<ModifierData> criticalConfirmationModifiers,
        List<ModifierData> criticalDamageModifiers,
        int totalAttackBonus,
        int totalDamageBonus,
        int totalCriticalConfirmationBonus,
        int totalCriticalDamageBonus
)
{
}
