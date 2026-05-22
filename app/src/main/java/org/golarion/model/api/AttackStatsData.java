package org.golarion.model.api;

import java.util.List;

public record AttackStatsData(
        List<AttackEntryData> attacks,
        List<AttackTypeModifierData> attackTypeModifiers
)
{
}
