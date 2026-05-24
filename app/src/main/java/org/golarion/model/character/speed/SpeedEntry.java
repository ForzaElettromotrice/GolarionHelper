package org.golarion.model.character.speed;

import lombok.NonNull;
import org.golarion.model.api.SpeedData;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class SpeedEntry implements ModifierTarget
{
    private static final double METERS_PER_UNIT = 1.5;
    private final List<Modifier> modifiers;
    private int baseUnits;

    public SpeedEntry()
    {
        this.baseUnits = 6;
        this.modifiers = new ArrayList<>();
    }

    public void setBaseUnits(int baseUnits)
    {
        if (baseUnits < 0)
        {
            throw new IllegalArgumentException("baseUnits must not be negative");
        }

        this.baseUnits = baseUnits;
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

    public void registerTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget("speed", this);
    }

    public SpeedData toData()
    {
        int totalUnits = getTotalUnits();
        return new SpeedData(
                baseUnits,
                totalUnits,
                toMeters(baseUnits),
                toMeters(totalUnits),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    private int getTotalUnits()
    {
        return Math.max(0, baseUnits + Modifier.calculateTotal(modifiers));
    }

    private double toMeters(int units)
    {
        return units * METERS_PER_UNIT;
    }
}
