package org.golarion.model.api;

import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.UUID;

public record ModifierData(
        UUID id,
        ModifierType modifierType,
        String source,
        boolean enabled,
        String description,
        BonusType bonusType,
        String expression
)
{
}
