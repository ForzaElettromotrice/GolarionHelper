package org.golarion.model.character.modifier;

import lombok.Getter;
import lombok.Setter;
import org.golarion.model.api.BonusData;

import java.util.*;

@Getter
public class Bonus
{
    public static int calculateTotal(List<Bonus> bonuses)
    {
        Map<BonusType, List<Bonus>> bonusesByType = new EnumMap<>(BonusType.class);

        for (Bonus bonus : bonuses)
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

    private static String validateSource(String source)
    {
        String normalizedSource = normalize(source);
        if (normalizedSource.isEmpty())
        {
            throw new IllegalArgumentException("source must not be blank");
        }

        return normalizedSource;
    }

    private static BonusType validateBonusType(BonusType bonusType)
    {
        if (bonusType == null)
        {
            throw new IllegalArgumentException("bonusType must not be null");
        }

        return bonusType;
    }

    private static int validateValue(int value)
    {
        if (value <= 0)
        {
            throw new IllegalArgumentException("value must be greater than 0");
        }

        return value;
    }

    private static String normalize(String value)
    {
        return value == null ? "" : value.trim();
    }

    private static int calculateBonusGroup(BonusType bonusType, List<Bonus> typedBonuses)
    {
        int total = switch (bonusType.getStackingRule())
        {
            case HIGHEST_ONLY -> typedBonuses.stream()
                    .mapToInt(Bonus::getValue)
                    .max()
                    .orElse(0);

            case STACKS -> typedBonuses.stream()
                    .mapToInt(Bonus::getValue)
                    .sum();

            case STACKS_UNLESS_SAME_SOURCE -> calculateBonusesThatStackUnlessSameSource(typedBonuses);
        };

        if (bonusType == BonusType.INHERENT)
        {
            return Math.min(total, 5);
        }

        return total;
    }

    private static int calculateBonusesThatStackUnlessSameSource(List<Bonus> typedBonuses)
    {
        Map<String, Integer> highestBonusBySource = new HashMap<>();

        for (Bonus bonus : typedBonuses)
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

    private final UUID id;
    private final String source;
    private final BonusType bonusType;
    private final int value;
    private final String description;
    @Setter
    private boolean enabled;

    public Bonus(String source, BonusType bonusType, int value)
    {
        this(source, bonusType, value, true, "");
    }

    public Bonus(String source, BonusType bonusType, int value, boolean enabled, String description)
    {
        this.id = UUID.randomUUID();
        this.source = validateSource(source);
        this.bonusType = validateBonusType(bonusType);
        this.value = validateValue(value);
        this.enabled = enabled;
        this.description = normalize(description);
    }

    public BonusData toData()
    {
        return new BonusData(id, source, bonusType, value, enabled, description);
    }
}
