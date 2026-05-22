package org.golarion.model.api;

import org.golarion.model.character.attack.AttackType;

import java.util.List;

public record AttackTypeModifierData(
        AttackType attackType,
        int totalBonus,
        List<ModifierData> modifiers
)
{
}
