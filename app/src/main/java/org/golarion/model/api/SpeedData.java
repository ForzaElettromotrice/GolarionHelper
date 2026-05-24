package org.golarion.model.api;

import java.util.List;

public record SpeedData(
        int baseUnits,
        int totalUnits,
        double baseMeters,
        double totalMeters,
        List<ModifierData> modifiers
)
{
}
