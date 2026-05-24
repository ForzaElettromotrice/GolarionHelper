package org.golarion.model.character.savingthrow;

import lombok.NonNull;
import org.golarion.model.api.SavingThrowData;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class SavingThrowEntry implements ModifierTarget
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

    public SavingThrowData toData(@NonNull SavingThrowType savingThrowType, int abilityModifier)
    {
        return new SavingThrowData(
                savingThrowType,
                baseValue,
                getTotalValue(abilityModifier),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    public void registerTargets(@NonNull TargetManager targetManager, @NonNull String targetName)
    {
        targetManager.registerModifierTarget(targetName, this);
    }

    private int getTotalValue(int abilityModifier)
    {
        return abilityModifier + baseValue + Modifier.calculateTotal(modifiers);
    }
}
