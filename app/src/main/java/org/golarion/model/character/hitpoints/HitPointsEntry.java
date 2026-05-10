package org.golarion.model.character.hitpoints;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.HitPointsData;

public class HitPointsEntry
{
    @Getter
    private int maxHp;
    private int currentHp;
    private int temporaryHp;
    private int nonlethalDamage;

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
            case MAX -> setMaxHp(maxHp + delta);
            case CURRENT -> changeCurrentHp(delta);
            case TEMPORARY -> changeTemporaryHp(delta);
            case NONLETHAL -> changeNonlethalDamage(delta);
        }
    }

    public HitPointsData toData()
    {
        return new HitPointsData(maxHp, currentHp, temporaryHp, nonlethalDamage);
    }

    private void changeNonlethalDamage(int delta)
    {
        int updatedNonlethalDamage = nonlethalDamage + delta;
        setNonlethalDamage(updatedNonlethalDamage);
    }

    private void setMaxHp(int maxHp)
    {
        this.maxHp = Math.max(0, maxHp);
        if (currentHp > this.maxHp)
        {
            currentHp = this.maxHp;
        }

        if (nonlethalDamage > this.maxHp)
        {
            int overflowDamage = nonlethalDamage - this.maxHp;
            nonlethalDamage = this.maxHp;
            subtractHitPoints(overflowDamage);
        }
    }

    private void setCurrentHp(int currentHp)
    {
        this.currentHp = Math.min(currentHp, maxHp);
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

        if (nonlethalDamage <= maxHp)
        {
            this.nonlethalDamage = nonlethalDamage;
            return;
        }

        this.nonlethalDamage = maxHp;
        subtractHitPoints(nonlethalDamage - maxHp);
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
