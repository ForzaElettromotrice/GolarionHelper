package org.golarion.model.api;

import java.util.List;

public record InitiativeData(
        int baseValue,
        int totalValue,
        List<BonusData> bonuses,
        List<PenaltyData> penalties
)
{
}
