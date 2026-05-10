package org.golarion.model.character.savingthrow;

import lombok.NonNull;
import org.golarion.model.api.SavingThrowData;
import org.golarion.model.character.modifier.Modifier;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class SavingThrowEntry
{
    private final List<Modifier> modifiers;
    private int baseValue;

    public SavingThrowEntry()
    {
        this.baseValue = 0;
        this.modifiers = new ArrayList<>();
    }

    public void setBaseValue(int baseValue)
    {
        if (baseValue < 0)
        {
            throw new IllegalArgumentException("baseValue must not be negative");
        }

        this.baseValue = baseValue;
    }

    public void addModifier(@NonNull Modifier modifier)
    {
        modifiers.add(modifier);
    }

    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
    }

    public void setModifierEnabled(@NonNull UUID modifierId, boolean enabled)
    {
        modifiers.stream()
                .filter(bonus -> bonus.getId().equals(modifierId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("modifierId not found: " + modifierId))
                .setEnabled(enabled);
    }

    public SavingThrowData toData(@NonNull SavingThrowType savingThrowType, int abilityModifier)
    {
        return new SavingThrowData(
                savingThrowType,
                baseValue,
                getTotalValue(abilityModifier),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    private int getTotalValue(int abilityModifier)
    {
        return abilityModifier + baseValue + Modifier.calculateTotal(modifiers);
    }
}
