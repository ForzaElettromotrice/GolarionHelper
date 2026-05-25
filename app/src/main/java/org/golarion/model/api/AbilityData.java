package org.golarion.model.api;

import org.golarion.model.character.ability.AbilityType;

public record AbilityData(
        AbilityType abilityType,
        int baseValue,
        int totalValue,
        int modifier,
        ModifierSetData modifiers
)
{
}
