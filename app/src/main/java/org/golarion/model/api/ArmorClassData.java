package org.golarion.model.api;

import java.util.List;

public record ArmorClassData(
        int totalValue,
        int touchValue,
        int flatFootedValue,
        List<BonusData> bonuses,
        List<PenaltyData> penalties
)
{
}
