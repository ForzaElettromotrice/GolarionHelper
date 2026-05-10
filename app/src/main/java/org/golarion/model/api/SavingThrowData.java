package org.golarion.model.api;

import org.golarion.model.character.savingthrow.SavingThrowType;

import java.util.List;

public record SavingThrowData(
        SavingThrowType savingThrowType,
        int baseValue,
        int totalValue,
        List<ModifierData> modifiers
)
{
}
