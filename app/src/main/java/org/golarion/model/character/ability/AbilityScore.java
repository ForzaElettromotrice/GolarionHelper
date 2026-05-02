package org.golarion.model.character.ability;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.AbilityData;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Penalty;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;


public class AbilityScore
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
    private final List<Bonus> bonuses;
    private final List<Penalty> penalties;
    @Getter
    private int baseValue;

    public AbilityScore(int baseValue)
    {
        this.baseValue = 10;
        this.bonuses = new ArrayList<>();
        this.penalties = new ArrayList<>();
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

    public void addBonus(@NonNull Bonus bonus)
    {
        if (!ALLOWED_BONUS_TYPES.contains(bonus.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + bonus.getBonusType() + " is not applicable to an ability score");
        }

        bonuses.add(bonus);
    }

    public void removeBonus(@NonNull UUID bonusId)
    {
        bonuses.removeIf(bonus -> bonus.getId().equals(bonusId));
    }

    public void setBonusEnabled(@NonNull UUID bonusId, boolean enabled)
    {
        findBonusById(bonusId).setEnabled(enabled);
    }

    public void addPenalty(@NonNull Penalty penalty)
    {

        penalties.add(penalty);
    }

    public void removePenalty(@NonNull UUID penaltyId)
    {
        penalties.removeIf(penalty -> penalty.getId().equals(penaltyId));
    }

    public void setPenaltyEnabled(@NonNull UUID penaltyId, boolean enabled)
    {
        findPenaltyById(penaltyId).setEnabled(enabled);
    }

    public AbilityData toData(@NonNull Ability ability)
    {
        return new AbilityData(ability, baseValue, getTotalValue(), getModifier(), getBonuses(), getPenalties());
    }

    public int getModifier()
    {
        return Math.floorDiv(getTotalValue() - 10, 2);
    }

    private List<BonusData> getBonuses()
    {
        return bonuses.stream().map(Bonus::toData).toList();
    }

    private List<PenaltyData> getPenalties()
    {
        return penalties.stream().map(Penalty::toData).toList();
    }

    private int getTotalValue()
    {
        return baseValue + getTotalBonus() - getTotalPenalty();
    }

    private int getTotalBonus()
    {
        return Bonus.calculateTotal(bonuses);
    }

    private int getTotalPenalty()
    {
        return penalties.stream()
                .filter(Penalty::isEnabled)
                .mapToInt(Penalty::getValue)
                .sum();
    }

    private Bonus findBonusById(@NonNull UUID bonusId)
    {
        return bonuses.stream()
                .filter(bonus -> bonus.getId().equals(bonusId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("bonusId not found: " + bonusId));
    }

    private Penalty findPenaltyById(@NonNull UUID penaltyId)
    {
        return penalties.stream()
                .filter(penalty -> penalty.getId().equals(penaltyId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("penaltyId not found: " + penaltyId));
    }
}
