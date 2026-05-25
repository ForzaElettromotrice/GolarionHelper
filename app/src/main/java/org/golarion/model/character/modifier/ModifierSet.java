package org.golarion.model.character.modifier;

import lombok.NonNull;
import org.golarion.model.api.ModifierData;
import org.golarion.model.api.ModifierSetData;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class ModifierSet
{
    private final List<Modifier> modifiers;
    private final List<ConditionalModifier> conditionalModifiers;

    public ModifierSet()
    {
        this.modifiers = new ArrayList<>();
        this.conditionalModifiers = new ArrayList<>();
    }

    public void addModifier(@NonNull Modifier modifier)
    {
        modifiers.add(modifier);
    }

    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
    }

    public void addConditionalModifier(@NonNull ConditionalModifier modifier)
    {
        conditionalModifiers.add(modifier);
    }

    public void removeConditionalModifier(@NonNull UUID modifierId)
    {
        conditionalModifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
    }

    public int calculateTotal()
    {
        return Modifier.calculateTotal(modifiers);
    }

    public ModifierSetData toData()
    {
        return new ModifierSetData(
                toModifierData(),
                toConditionalModifierData()
        );
    }

    public List<ModifierData> toModifierData()
    {
        return modifiers.stream().map(Modifier::toData).toList();
    }

    public List<org.golarion.model.api.ConditionalModifierData> toConditionalModifierData()
    {
        return conditionalModifiers.stream()
                .map(modifier -> modifier.toData(modifiers))
                .filter(data -> data.displayValue() != 0)
                .toList();
    }

    public List<Modifier> getModifiers()
    {
        return List.copyOf(modifiers);
    }

    public List<ConditionalModifier> getConditionalModifiers()
    {
        return List.copyOf(conditionalModifiers);
    }
}
