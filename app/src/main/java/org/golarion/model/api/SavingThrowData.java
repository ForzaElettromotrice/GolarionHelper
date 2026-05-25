package org.golarion.model.api;

import org.golarion.model.character.savingthrow.SavingThrowType;

public record SavingThrowData(
        SavingThrowType savingThrowType,
        int baseValue,
        int totalValue,
        ModifierSetData modifiers
)
{
}
