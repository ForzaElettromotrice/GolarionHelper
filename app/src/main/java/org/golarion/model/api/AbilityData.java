package org.golarion.model.api;

import org.golarion.model.character.ability.AbilityType;

import java.util.List;

public record AbilityData(
        AbilityType abilityType,
        int baseValue,
        int totalValue,
        int modifier,
        List<ModifierData> modifiers
)
{
}
