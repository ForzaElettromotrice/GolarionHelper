package org.golarion.model.character.equipment;

import lombok.NonNull;
import org.golarion.model.character.action.Action;
import org.golarion.model.character.action.ApplyCarryingLoadAction;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.function.IntSupplier;

public class WeightEntry implements ModifierTarget
{
    private static final long[] LIGHT_LOAD_MAX_GRAMS = {
            0,
            1500, 3000, 5000, 6500, 8000, 10000, 11500, 13000, 15000, 16500,
            19000, 21500, 25000, 29000, 33000, 38000, 43000, 50000, 58000, 66500,
            76500, 86500, 100000, 116500, 133000, 153000, 173000, 200000, 233000
    };
    private static final long[] MEDIUM_LOAD_MAX_GRAMS = {
            0,
            3000, 6500, 10000, 13000, 16500, 20000, 23000, 26500, 30000, 33000,
            38000, 43000, 50000, 58000, 66500, 76500, 86500, 100000, 116500, 133000,
            153000, 173000, 200000, 233000, 266500, 306500, 346500, 400000, 466500
    };
    private static final long[] HEAVY_LOAD_MAX_GRAMS = {
            0,
            5000, 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000,
            57500, 65000, 75000, 87500, 100000, 115000, 130000, 150000, 175000, 200000,
            230000, 260000, 300000, 350000, 400000, 460000, 520000, 600000, 700000
    };

    private final List<Modifier> modifiers;
    private IntSupplier strengthResolver;
    private double carryingCapacityMultiplier;
    private long carriedWeightGrams;

    public WeightEntry()
    {
        this.modifiers = new ArrayList<>();
        this.strengthResolver = () -> 10;
        this.carryingCapacityMultiplier = 1.0;
    }

    public void setStrengthResolver(@NonNull IntSupplier strengthResolver)
    {
        this.strengthResolver = strengthResolver;
    }

    public void setCarryingCapacityMultiplier(double carryingCapacityMultiplier)
    {
        if (carryingCapacityMultiplier <= 0)
        {
            throw new IllegalArgumentException("carryingCapacityMultiplier must be positive");
        }

        this.carryingCapacityMultiplier = carryingCapacityMultiplier;
    }

    public void changeCarryingCapacityMultiplier(double multiplier)
    {
        if (multiplier <= 0)
        {
            throw new IllegalArgumentException("multiplier must be positive");
        }

        setCarryingCapacityMultiplier(carryingCapacityMultiplier * multiplier);
    }

    public Action setCarriedWeightGrams(long carriedWeightGrams)
    {
        if (carriedWeightGrams < 0)
        {
            throw new IllegalArgumentException("carriedWeightGrams must not be negative");
        }

        this.carriedWeightGrams = carriedWeightGrams;
        return createCarryingLoadAction();
    }

    public Action changeWeightGrams(long delta)
    {
        try
        {
            return setCarriedWeightGrams(Math.addExact(carriedWeightGrams, delta));
        } catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("carriedWeightGrams is too large", exception);
        }
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
        targetManager.registerModifierTarget("carryingCapacityStrength", this);
    }

    public int getEffectiveStrength()
    {
        return Math.max(1, strengthResolver.getAsInt() + Modifier.calculateTotal(modifiers));
    }

    public List<Modifier> getModifiers()
    {
        return List.copyOf(modifiers);
    }

    public long getLightLoadMaxGrams()
    {
        return getCapacityForCurrentCreature(LIGHT_LOAD_MAX_GRAMS);
    }

    public long getMediumLoadMaxGrams()
    {
        return getCapacityForCurrentCreature(MEDIUM_LOAD_MAX_GRAMS);
    }

    public long getHeavyLoadMaxGrams()
    {
        return getCapacityForCurrentCreature(HEAVY_LOAD_MAX_GRAMS);
    }

    public CarryingLoad getCarryingLoad()
    {
        if (carriedWeightGrams <= getLightLoadMaxGrams())
        {
            return CarryingLoad.LIGHT;
        }
        if (carriedWeightGrams <= getMediumLoadMaxGrams())
        {
            return CarryingLoad.MEDIUM;
        }
        if (carriedWeightGrams <= getHeavyLoadMaxGrams())
        {
            return CarryingLoad.HEAVY;
        }

        return CarryingLoad.OVERLOADED;
    }

    private Action createCarryingLoadAction()
    {
        return new ApplyCarryingLoadAction(getCarryingLoad());
    }

    private long getCapacityForCurrentCreature(long[] capacityTable)
    {
        long mediumTallCapacity = getCapacityForStrength(getEffectiveStrength(), capacityTable);
        return Math.round(mediumTallCapacity * carryingCapacityMultiplier);
    }

    private long getCapacityForStrength(int strength, long[] capacityTable)
    {
        int normalizedStrength = strength;
        long multiplier = 1;

        while (normalizedStrength > 29)
        {
            normalizedStrength -= 10;
            multiplier = multiplyExact(multiplier, 4);
        }

        return multiplyExact(capacityTable[normalizedStrength], multiplier);
    }

    private long multiplyExact(long left, long right)
    {
        try
        {
            return Math.multiplyExact(left, right);
        } catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("carrying capacity is too large", exception);
        }
    }
}
