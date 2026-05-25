package org.golarion.model.api;

import java.util.List;

public record ModifierSetData(
        List<ModifierData> modifiers,
        List<ConditionalModifierData> conditionalModifiers
)
{
}
