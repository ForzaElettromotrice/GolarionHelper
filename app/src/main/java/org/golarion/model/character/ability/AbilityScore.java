package org.golarion.model.character.ability;

import lombok.NonNull;
import org.golarion.model.api.AbilityData;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Modifiable;
import org.golarion.model.character.modifier.Modifier;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;


public class AbilityScore implements Modifiable
{
    private static final EnumSet<BonusType> ALLOWED_BONUS_TYPES = EnumSet.of(
            BonusType.ALCHEMICAL,
            BonusType.LUCK,
            BonusType.INHERENT,
            BonusType.MORALE,
            BonusType.ENHANCEMENT,
            BonusType.PROFANE,
            BonusType.RACIAL,
            BonusType.SACRED,
            BonusType.SIZE
    );
    private final List<Modifier> modifiers;
    private int baseValue;

    public AbilityScore(int baseValue)
    {
        this.baseValue = 10;
        this.modifiers = new ArrayList<>();
        setBaseValue(baseValue);
    }

    public void setBaseValue(int baseValue)
    {
        if (baseValue <= 0)
        {
            throw new IllegalArgumentException("baseValue must be greater than 0");
        }

        this.baseValue = baseValue;
    }

    @Override
    public void addModifier(@NonNull Modifier modifier)
    {
        if (modifier.getBonusType() != null && !ALLOWED_BONUS_TYPES.contains(modifier.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + modifier.getBonusType() + " is not applicable to an ability score");
        }

        modifiers.add(modifier);
    }

    @Override
    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
    }

    public AbilityData toData(@NonNull AbilityType abilityType)
    {
        return new AbilityData(
                abilityType,
                baseValue,
                getTotalValue(),
                getModifier(),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    public int getModifier()
    {
        return Math.floorDiv(getTotalValue() - 10, 2);
    }

    public int getTotalValue()
    {
        return baseValue + Modifier.calculateTotal(modifiers);
    }
}
