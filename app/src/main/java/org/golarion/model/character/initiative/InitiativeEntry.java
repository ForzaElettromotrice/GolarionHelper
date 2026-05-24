package org.golarion.model.character.initiative;

import lombok.NonNull;
import org.golarion.model.api.InitiativeData;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class InitiativeEntry implements ModifierTarget
{
    private final List<Modifier> modifiers;
    private int baseValue;

    public InitiativeEntry()
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
        modifiers.removeIf(bonus -> bonus.getId().equals(modifierId));
    }

    public void setModifierEnabled(@NonNull UUID modifierId, boolean enabled)
    {
        modifiers.stream()
                .filter(bonus -> bonus.getId().equals(modifierId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("modifierId not found: " + modifierId))
                .setEnabled(enabled);
    }

    public InitiativeData toData(int abilityModifier)
    {
        return new InitiativeData(
                baseValue,
                getTotalValue(abilityModifier),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    public void registerTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget("initiative", this);
    }

    private int getTotalValue(int abilityModifier)
    {
        return abilityModifier + baseValue + Modifier.calculateTotal(modifiers);
    }
}
