package org.golarion.model.api;

import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.EffectEntryType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.UUID;

public record EffectEntryData(
        UUID id,
        EffectEntryType entryType,
        String targetName,
        ModifierType modifierType,
        BonusType bonusType,
        String source,
        boolean enabled,
        String description,
        String expression
)
{
}
