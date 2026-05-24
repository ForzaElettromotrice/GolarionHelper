package org.golarion.model.api;

import org.golarion.model.character.ability.AbilityType;

import java.util.List;

public record CombatManeuverData(
        AbilityType abilityType,
        int totalValue,
        List<ModifierData> modifiers
)
{
}
