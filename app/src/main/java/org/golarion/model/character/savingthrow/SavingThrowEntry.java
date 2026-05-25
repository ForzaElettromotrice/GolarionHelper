package org.golarion.model.character.savingthrow;

import lombok.NonNull;
import org.golarion.model.api.SavingThrowData;
import org.golarion.model.character.modifier.ConditionalModifier;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierSet;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.UUID;

public class SavingThrowEntry implements ModifierTarget
{
    private final ModifierSet modifiers;
    private int baseValue;

    public SavingThrowEntry()
    {
        this.baseValue = 0;
        this.modifiers = new ModifierSet();
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
        modifiers.addModifier(modifier);
    }

    @Override
    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeModifier(modifierId);
    }

    @Override
    public void addConditionalModifier(@NonNull ConditionalModifier modifier)
    {
        modifiers.addConditionalModifier(modifier);
    }

    @Override
    public void removeConditionalModifier(@NonNull UUID modifierId)
    {
        modifiers.removeConditionalModifier(modifierId);
    }

    public SavingThrowData toData(@NonNull SavingThrowType savingThrowType, int abilityModifier)
    {
        return new SavingThrowData(
                savingThrowType,
                baseValue,
                getTotalValue(abilityModifier),
                modifiers.toData()
        );
    }

    public void registerTargets(@NonNull TargetManager targetManager, @NonNull String targetName)
    {
        targetManager.registerModifierTarget(targetName, this);
    }

    private int getTotalValue(int abilityModifier)
    {
        return abilityModifier + baseValue + modifiers.calculateTotal();
    }
}
