package org.golarion.model.character.ability;

import lombok.Getter;
import org.golarion.model.character.modifier.BonusType;

import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;

@Getter
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

    private int baseValue;
    private final List<AbilityBonus> bonuses;
    private final List<AbilityPenalty> penalties;

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

    public List<AbilityBonus> getBonuses()
    {
        return Collections.unmodifiableList(bonuses);
    }

    public List<AbilityPenalty> getPenalties()
    {
        return Collections.unmodifiableList(penalties);
    }

    public void addBonus(AbilityBonus bonus)
    {
        if (bonus == null)
        {
            throw new IllegalArgumentException("bonus must not be null");
        }
        if (!ALLOWED_BONUS_TYPES.contains(bonus.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + bonus.getBonusType() + " is not applicable to an ability score");
        }

        bonuses.add(bonus);
    }

    public void removeBonus(UUID bonusId)
    {
        if (bonusId == null)
        {
            throw new IllegalArgumentException("bonusId must not be null");
        }

        bonuses.removeIf(bonus -> bonus.getId().equals(bonusId));
    }

    public void setBonusEnabled(UUID bonusId, boolean enabled)
    {
        findBonusById(bonusId).setEnabled(enabled);
    }

    public void addPenalty(AbilityPenalty penalty)
    {
        if (penalty == null)
        {
            throw new IllegalArgumentException("penalty must not be null");
        }

        penalties.add(penalty);
    }

    public void removePenalty(UUID penaltyId)
    {
        if (penaltyId == null)
        {
            throw new IllegalArgumentException("penaltyId must not be null");
        }

        penalties.removeIf(penalty -> penalty.getId().equals(penaltyId));
    }

    public void setPenaltyEnabled(UUID penaltyId, boolean enabled)
    {
        findPenaltyById(penaltyId).setEnabled(enabled);
    }

    public int getTotalValue()
    {
        return baseValue + calculateTotalBonus() - calculateTotalPenalty();
    }

    public int getModifier()
    {
        return Math.floorDiv(getTotalValue() - 10, 2);
    }

    private int calculateTotalBonus()
    {
        Map<BonusType, List<AbilityBonus>> bonusesByType = new EnumMap<>(BonusType.class);

        for (AbilityBonus bonus : bonuses)
        {
            if (!bonus.isEnabled())
            {
                continue;
            }

            bonusesByType
                .computeIfAbsent(bonus.getBonusType(), ignored -> new ArrayList<>())
                .add(bonus);
        }

        return bonusesByType.entrySet().stream()
            .mapToInt(entry -> calculateBonusGroup(entry.getKey(), entry.getValue()))
            .sum();
    }

    private int calculateBonusGroup(BonusType bonusType, List<AbilityBonus> typedBonuses)
    {
        int total = switch (bonusType.getStackingRule())
        {
            case HIGHEST_ONLY -> typedBonuses.stream()
                .mapToInt(AbilityBonus::getValue)
                .max()
                .orElse(0);

            case STACKS -> typedBonuses.stream()
                .mapToInt(AbilityBonus::getValue)
                .sum();

            case STACKS_UNLESS_SAME_SOURCE -> calculateBonusesThatStackUnlessSameSource(typedBonuses);
        };

        if (bonusType == BonusType.INHERENT)
        {
            return Math.min(total, 5);
        }

        return total;
    }

    private int calculateBonusesThatStackUnlessSameSource(List<AbilityBonus> typedBonuses)
    {
        Map<String, Integer> highestBonusBySource = new java.util.HashMap<>();

        for (AbilityBonus bonus : typedBonuses)
        {
            highestBonusBySource.merge(
                bonus.getSource(),
                bonus.getValue(),
                Math::max
            );
        }

        return highestBonusBySource.values().stream()
            .mapToInt(Integer::intValue)
            .sum();
    }

    private int calculateTotalPenalty()
    {
        return penalties.stream()
            .filter(AbilityPenalty::isEnabled)
            .mapToInt(AbilityPenalty::getValue)
            .sum();
    }

    private AbilityBonus findBonusById(UUID bonusId)
    {
        if (bonusId == null)
        {
            throw new IllegalArgumentException("bonusId must not be null");
        }

        return bonuses.stream()
            .filter(bonus -> bonus.getId().equals(bonusId))
            .findFirst()
            .orElseThrow(() -> new IllegalArgumentException("bonusId not found: " + bonusId));
    }

    private AbilityPenalty findPenaltyById(UUID penaltyId)
    {
        if (penaltyId == null)
        {
            throw new IllegalArgumentException("penaltyId must not be null");
        }

        return penalties.stream()
            .filter(penalty -> penalty.getId().equals(penaltyId))
            .findFirst()
            .orElseThrow(() -> new IllegalArgumentException("penaltyId not found: " + penaltyId));
    }

    private void validateBonusTypeApplicability(BonusType bonusType)
    {
        if (!ALLOWED_BONUS_TYPES.contains(bonusType))
        {
            throw new IllegalArgumentException("bonusType " + bonusType + " is not applicable to an ability score");
        }
    }
}
