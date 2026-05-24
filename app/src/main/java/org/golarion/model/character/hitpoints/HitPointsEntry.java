package org.golarion.model.character.hitpoints;

import lombok.NonNull;
import org.golarion.model.api.HitPointsData;
import org.golarion.model.character.modifier.TargetManager;

import java.util.function.IntSupplier;

public class HitPointsEntry
{
    @NonNull
    private IntSupplier maxHpModifierResolver;
    private int baseMaxHp;
    private int currentHp;
    private int temporaryHp;
    private int nonlethalDamage;

    public HitPointsEntry()
    {
        this.maxHpModifierResolver = () -> 0;
    }

    public void setMaxHpModifierResolver(@NonNull IntSupplier maxHpModifierResolver)
    {
        this.maxHpModifierResolver = maxHpModifierResolver;
        normalizeToMaxHp();
    }

    public void set(@NonNull HitPointField field, int value)
    {
        switch (field)
        {
            case MAX -> setMaxHp(value);
            case CURRENT -> setCurrentHp(value);
            case TEMPORARY -> setTemporaryHp(value);
            case NONLETHAL -> setNonlethalDamage(value);
        }
    }

    public void change(@NonNull HitPointField field, int delta)
    {
        switch (field)
        {
            case MAX -> setMaxHp(baseMaxHp + delta);
            case CURRENT -> changeCurrentHp(delta);
            case TEMPORARY -> changeTemporaryHp(delta);
            case NONLETHAL -> changeNonlethalDamage(delta);
        }
    }

    public HitPointsData toData()
    {
        normalizeToMaxHp();
        return new HitPointsData(getMaxHp(), currentHp, temporaryHp, nonlethalDamage);
    }

    public void registerDeltaTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerDeltaTarget("maxHp", delta -> change(HitPointField.MAX, delta));
        targetManager.registerDeltaTarget("currentHp", delta -> change(HitPointField.CURRENT, delta));
        targetManager.registerDeltaTarget("temporaryHp", delta -> change(HitPointField.TEMPORARY, delta));
        targetManager.registerDeltaTarget("nonlethalDamage", delta -> change(HitPointField.NONLETHAL, delta));
    }

    private void changeNonlethalDamage(int delta)
    {
        int updatedNonlethalDamage = nonlethalDamage + delta;
        setNonlethalDamage(updatedNonlethalDamage);
    }

    private void setMaxHp(int maxHp)
    {
        this.baseMaxHp = Math.max(0, maxHp);
        normalizeToMaxHp();
    }

    private void normalizeToMaxHp()
    {
        int maxHp = getMaxHp();
        if (currentHp > maxHp)
        {
            currentHp = maxHp;
        }

        if (nonlethalDamage > maxHp)
        {
            int overflowDamage = nonlethalDamage - maxHp;
            nonlethalDamage = maxHp;
            subtractHitPoints(overflowDamage);
        }
    }

    private void setCurrentHp(int currentHp)
    {
        this.currentHp = Math.min(currentHp, getMaxHp());
    }

    private void changeCurrentHp(int delta)
    {
        if (delta >= 0)
        {
            setCurrentHp(currentHp + delta);
            return;
        }

        subtractHitPoints(-delta);
    }

    private void setTemporaryHp(int temporaryHp)
    {
        if (temporaryHp < 0)
        {
            this.temporaryHp = 0;
            subtractHitPoints(-temporaryHp);
            return;
        }
        this.temporaryHp = temporaryHp;
    }

    private void changeTemporaryHp(int delta)
    {
        if (delta >= 0)
        {
            temporaryHp += delta;
            return;
        }

        subtractHitPoints(-delta);
    }

    private void setNonlethalDamage(int nonlethalDamage)
    {
        if (nonlethalDamage < 0)
        {
            this.nonlethalDamage = 0;
            return;
        }

        int maxHp = getMaxHp();
        if (nonlethalDamage <= maxHp)
        {
            this.nonlethalDamage = nonlethalDamage;
            return;
        }

        this.nonlethalDamage = maxHp;
        subtractHitPoints(nonlethalDamage - maxHp);
    }

    private int getMaxHp()
    {
        return Math.max(0, baseMaxHp + maxHpModifierResolver.getAsInt());
    }

    private void subtractHitPoints(int amount)
    {
        int remainingDamage = amount;

        if (temporaryHp > 0)
        {
            int absorbedByTemporaryHp = Math.min(temporaryHp, remainingDamage);
            temporaryHp -= absorbedByTemporaryHp;
            remainingDamage -= absorbedByTemporaryHp;
        }

        if (remainingDamage > 0)
        {
            currentHp -= remainingDamage;
        }
    }
}
