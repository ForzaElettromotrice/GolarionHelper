package org.golarion.model.character.modifier;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.ModifierData;

import java.util.*;

public class Modifier
{
    public static int calculateTotal(@NonNull List<Modifier> modifiers)
    {
        Map<BonusType, List<Modifier>> bonusesByType = new EnumMap<>(BonusType.class);
        List<Modifier> penalties = new ArrayList<>();

        for (Modifier modifier : modifiers)
        {
            if (!modifier.isEnabled())
            {
                continue;
            }
            if (modifier.type == ModifierType.PENALTY)
            {
                penalties.add(modifier);
                continue;
            }
            bonusesByType
                    .computeIfAbsent(modifier.getBonusType(), ignored -> new ArrayList<>())
                    .add(modifier);
        }

        return bonusesByType.entrySet().stream()
                .mapToInt(entry -> calculateBonusGroup(entry.getKey(), entry.getValue()))
                .sum()
                - penalties.stream()
                .mapToInt(Modifier::getValue)
                .sum();
    }

    private static int calculateBonusGroup(@NonNull BonusType bonusType, @NonNull List<Modifier> typedBonuses)
    {
        int total = switch (bonusType.getStackingRule())
        {
            case HIGHEST_ONLY -> typedBonuses.stream()
                    .mapToInt(Modifier::getValue)
                    .max()
                    .orElse(0);

            case STACKS -> typedBonuses.stream()
                    .mapToInt(Modifier::getValue)
                    .sum();

            case STACKS_UNLESS_SAME_SOURCE -> calculateBonusesThatStackUnlessSameSource(typedBonuses);
        };

        if (bonusType == BonusType.INHERENT)
        {
            return Math.min(total, 5);
        }

        return total;
    }

    private static int calculateBonusesThatStackUnlessSameSource(@NonNull List<Modifier> typedBonuses)
    {
        Map<String, Integer> highestBonusBySource = new HashMap<>();

        for (Modifier bonus : typedBonuses)
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

    private static String validateSource(@NonNull String source)
    {
        String normalizedSource = source.trim();
        if (normalizedSource.isEmpty())
        {
            throw new IllegalArgumentException("source must not be blank");
        }

        return normalizedSource;
    }

    @Getter
    private final UUID id;
    @Getter
    private final ModifierType type;
    @Setter
    @Getter
    private String source;
    @Setter
    @Getter
    private boolean enabled;
    @Setter
    @Getter
    private String description;
    @Setter
    @Getter
    private BonusType bonusType;
    @Setter
    @Getter
    private Expression expression;

    public Modifier(@NonNull ModifierType type, @NonNull String source, boolean enabled, @NonNull String description, BonusType bonusType, @NonNull Expression expression)
    {
        if (type == ModifierType.BONUS && bonusType == null)
            throw new IllegalArgumentException("bonusType must not be null for bonus modifier");

        this.id = UUID.randomUUID();
        this.type = type;
        this.source = validateSource(source);
        this.enabled = enabled;
        this.description = description;
        this.bonusType = bonusType;
        this.expression = expression;
    }

    public ModifierData toData()
    {
        return new ModifierData(id, type, source, enabled, description, bonusType, expression.getExpression());
    }

    public int getValue()
    {
        int value = expression.getValue();
        if (type == ModifierType.BONUS && value < 0)
        {
            throw new IllegalArgumentException("bonus value must not be negative");
        }

        return Math.abs(value);
    }
}
