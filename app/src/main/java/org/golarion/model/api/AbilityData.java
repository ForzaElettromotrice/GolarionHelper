package org.golarion.model.api;

import org.golarion.model.character.ability.Ability;

import java.util.List;

public record AbilityData(
        Ability ability,
        int baseValue,
        int totalValue,
        int modifier,
        List<BonusData> bonuses,
        List<PenaltyData> penalties
)
{
}
