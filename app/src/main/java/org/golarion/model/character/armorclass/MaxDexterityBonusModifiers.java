package org.golarion.model.character.armorclass;

import lombok.NonNull;
import org.golarion.model.api.ModifierData;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

class MaxDexterityBonusModifiers implements ModifierTarget
{
    private final List<Modifier> modifiers;

    MaxDexterityBonusModifiers()
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

    int getTotalValue()
    {
        return Modifier.calculateTotal(modifiers);
    }

    List<ModifierData> toData()
    {
        return modifiers.stream().map(Modifier::toData).toList();
    }
}
