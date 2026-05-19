package org.golarion.model.api;

import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.spell.SpellSavingThrowEffect;

public record SpellSavingThrowData(
        SavingThrowType savingThrowType,
        SpellSavingThrowEffect effect,
        String description
)
{
    public SpellSavingThrowData
    {
        if (savingThrowType == null)
        {
            throw new IllegalArgumentException("savingThrowType must not be null");
        }
        if (effect == null)
        {
            throw new IllegalArgumentException("effect must not be null");
        }
        if (description == null)
        {
            throw new IllegalArgumentException("description must not be null");
        }

        String normalizedDescription = description.trim();
        if (normalizedDescription.isBlank())
        {
            throw new IllegalArgumentException("description must not be blank");
        }

        description = normalizedDescription;
    }
}
