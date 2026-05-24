package org.golarion.model.api;

import java.util.List;

public record ArmorClassData(
        int totalValue,
        int touchValue,
        int flatFootedValue,
        Integer maxDexterityBonus,
        List<ModifierData> maxDexterityBonusModifiers,
        List<ModifierData> modifiers
)
{
}
