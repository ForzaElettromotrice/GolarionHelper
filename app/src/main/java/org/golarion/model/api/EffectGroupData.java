package org.golarion.model.api;

import java.util.List;
import java.util.UUID;

public record EffectGroupData(
        UUID id,
        String name,
        List<EffectEntryData> entries
)
{
}
