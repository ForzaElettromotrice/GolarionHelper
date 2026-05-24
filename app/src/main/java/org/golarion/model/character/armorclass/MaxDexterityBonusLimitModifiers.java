package org.golarion.model.character.armorclass;

import lombok.NonNull;
import org.golarion.model.api.ModifierData;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

class MaxDexterityBonusLimitModifiers implements ModifierTarget
{
    private final List<Modifier> modifiers;

    MaxDexterityBonusLimitModifiers()
    {
        this.modifiers = new ArrayList<>();
    }

    @Override
    public void addModifier(@NonNull Modifier modifier)
    {
        modifiers.add(modifier);
    }

    @Override
    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
    }

    Integer getLowestLimit()
    {
        return modifiers.stream()
                .filter(Modifier::isEnabled)
                .mapToInt(Modifier::getValue)
                .min()
                .stream()
                .boxed()
                .findFirst()
                .orElse(null);
    }

    List<ModifierData> toData()
    {
        return modifiers.stream().map(Modifier::toData).toList();
    }
}
