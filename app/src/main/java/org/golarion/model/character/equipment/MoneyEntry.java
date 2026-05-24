package org.golarion.model.character.equipment;

import lombok.NonNull;
import org.golarion.model.api.MoneyData;
import org.golarion.model.character.modifier.TargetManager;

public class MoneyEntry
{
    private static final int COIN_WEIGHT_GRAMS = 10;
    private long copper;
    private long silver;
    private long gold;
    private long platinum;

    public void set(@NonNull Currency currency, long amount)
    {
        if (amount < 0)
        {
            throw new IllegalArgumentException("amount must not be negative");
        }

        switch (currency)
        {
            case COPPER -> copper = amount;
            case SILVER -> silver = amount;
            case GOLD -> gold = amount;
            case PLATINUM -> platinum = amount;
        }
    }

    public void change(@NonNull Currency currency, long delta)
    {
        if (delta >= 0)
        {
            set(currency, addExact(getAmount(currency), delta));
            return;
        }

        remove(currency, negateExact(delta));
    }

    public MoneyData toData()
    {
        return new MoneyData(platinum, gold, silver, copper);
    }

    public void registerDeltaTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerDeltaTarget("copper", delta -> change(Currency.COPPER, delta));
        targetManager.registerDeltaTarget("silver", delta -> change(Currency.SILVER, delta));
        targetManager.registerDeltaTarget("gold", delta -> change(Currency.GOLD, delta));
        targetManager.registerDeltaTarget("platinum", delta -> change(Currency.PLATINUM, delta));
    }

    public long getTotalWeightGrams()
    {
        try
        {
            long coinCount = copper;
            coinCount = Math.addExact(coinCount, silver);
            coinCount = Math.addExact(coinCount, gold);
            coinCount = Math.addExact(coinCount, platinum);
            return Math.multiplyExact(coinCount, COIN_WEIGHT_GRAMS);
        }
        catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("money weight is too large", exception);
        }
    }

    private long getAmount(@NonNull Currency currency)
    {
        return switch (currency)
        {
            case COPPER -> copper;
            case SILVER -> silver;
            case GOLD -> gold;
            case PLATINUM -> platinum;
        };
    }

    private void remove(@NonNull Currency currency, long amount)
    {
        if (getAvailableAmount(currency) < amount)
        {
            throw new IllegalArgumentException("money cannot be negative");
        }

        while (getAmount(currency) < amount)
        {
            convertHigherCoin(currency);
        }

        set(currency, getAmount(currency) - amount);
    }

    private void convertHigherCoin(@NonNull Currency currency)
    {
        switch (currency)
        {
            case COPPER -> convertToCopper();
            case SILVER -> convertToSilver();
            case GOLD -> convertToGold();
            case PLATINUM -> throw new IllegalArgumentException("money cannot be negative");
        }
    }

    private void convertToCopper()
    {
        if (silver == 0)
        {
            convertToSilver();
        }

        silver--;
        copper = addExact(copper, Currency.SILVER.getCopperValue());
    }

    private void convertToSilver()
    {
        if (gold == 0)
        {
            convertToGold();
        }

        gold--;
        silver = addExact(silver, Currency.GOLD.getCopperValue() / Currency.SILVER.getCopperValue());
    }

    private void convertToGold()
    {
        if (platinum == 0)
        {
            throw new IllegalArgumentException("money cannot be negative");
        }

        platinum--;
        gold = addExact(gold, Currency.PLATINUM.getCopperValue() / Currency.GOLD.getCopperValue());
    }

    private long getAvailableAmount(@NonNull Currency currency)
    {
        try
        {
            return switch (currency)
            {
                case COPPER -> Math.addExact(
                        Math.addExact(copper, Math.multiplyExact(silver, Currency.SILVER.getCopperValue())),
                        Math.addExact(Math.multiplyExact(gold, Currency.GOLD.getCopperValue()), Math.multiplyExact(platinum, Currency.PLATINUM.getCopperValue()))
                );
                case SILVER -> Math.addExact(silver, Math.addExact(
                        Math.multiplyExact(gold, Currency.GOLD.getCopperValue() / Currency.SILVER.getCopperValue()),
                        Math.multiplyExact(platinum, Currency.PLATINUM.getCopperValue() / Currency.SILVER.getCopperValue())
                ));
                case GOLD -> Math.addExact(gold, Math.multiplyExact(platinum, Currency.PLATINUM.getCopperValue() / Currency.GOLD.getCopperValue()));
                case PLATINUM -> platinum;
            };
        }
        catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("money value is too large", exception);
        }
    }

    private long addExact(long left, long right)
    {
        try
        {
            return Math.addExact(left, right);
        }
        catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("money value is too large", exception);
        }
    }

    private long negateExact(long value)
    {
        try
        {
            return Math.negateExact(value);
        }
        catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("money value is too large", exception);
        }
    }
}
