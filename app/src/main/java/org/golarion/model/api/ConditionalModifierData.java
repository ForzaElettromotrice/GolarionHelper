package org.golarion.model.api;

import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.UUID;

public record ConditionalModifierData(
        UUID id,
        ModifierType modifierType,
        String source,
        String description,
        String condition,
        BonusType bonusType,
        String expression,
        int displayValue
)
{
}
