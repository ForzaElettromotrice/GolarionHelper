package org.golarion.model.character.modifier;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.ConditionalModifierData;

import java.util.List;
import java.util.UUID;

public class ConditionalModifier
{
    @Getter
    private final UUID id;
    @Getter
    private final ModifierType type;
    @Setter
    @Getter
    private String source;
    @Setter
    @Getter
    private String description;
    @Setter
    @Getter
    private String condition;
    @Setter
    @Getter
    private BonusType bonusType;
    @Setter
    @Getter
    private Expression expression;

    public ConditionalModifier(
            @NonNull ModifierType type,
            @NonNull String source,
            @NonNull String description,
            @NonNull String condition,
            BonusType bonusType,
            @NonNull Expression expression)
    {
        if (type == ModifierType.BONUS && bonusType == null)
        {
            throw new IllegalArgumentException("bonusType must not be null for bonus modifier");
        }

        this.id = UUID.randomUUID();
        this.type = type;
        this.source = validateRequiredText(source, "source");
        this.description = validateRequiredText(description, "description");
        this.condition = validateRequiredText(condition, "condition");
        this.bonusType = bonusType;
        this.expression = expression;
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

    public ConditionalModifierData toData(@NonNull List<Modifier> modifiers)
    {
        return new ConditionalModifierData(
                id,
                type,
                source,
                description,
                condition,
                bonusType,
                expression.getExpression(),
                getDisplayValue(modifiers)
        );
    }

    private int getDisplayValue(@NonNull List<Modifier> modifiers)
    {
        if (type == ModifierType.PENALTY)
        {
            return getValue();
        }

        return switch (bonusType.getStackingRule())
        {
            case STACKS -> getValue();
            case HIGHEST_ONLY -> Math.max(0, getEffectiveValue() - getHighestEnabledBonus(modifiers));
            case STACKS_UNLESS_SAME_SOURCE -> Math.max(0, getValue() - getHighestEnabledBonusFromSameSource(modifiers));
        };
    }

    private int getHighestEnabledBonus(@NonNull List<Modifier> modifiers)
    {
        int highestBonus = modifiers.stream()
                .filter(Modifier::isEnabled)
                .filter(modifier -> modifier.getType() == ModifierType.BONUS)
                .filter(modifier -> modifier.getBonusType() == bonusType)
                .mapToInt(Modifier::getValue)
                .max()
                .orElse(0);

        return getEffectiveBonusValue(highestBonus);
    }

    private int getHighestEnabledBonusFromSameSource(@NonNull List<Modifier> modifiers)
    {
        return modifiers.stream()
                .filter(Modifier::isEnabled)
                .filter(modifier -> modifier.getType() == ModifierType.BONUS)
                .filter(modifier -> modifier.getBonusType() == bonusType)
                .filter(modifier -> modifier.getSource().equals(source))
                .mapToInt(Modifier::getValue)
                .max()
                .orElse(0);
    }

    private int getEffectiveValue()
    {
        return getEffectiveBonusValue(getValue());
    }

    private int getEffectiveBonusValue(int value)
    {
        if (bonusType == BonusType.INHERENT)
        {
            return Math.min(value, 5);
        }

        return value;
    }

    private String validateRequiredText(@NonNull String value, @NonNull String fieldName)
    {
        String normalizedValue = value.trim();
        if (normalizedValue.isBlank())
        {
            throw new IllegalArgumentException(fieldName + " must not be blank");
        }

        return normalizedValue;
    }
}
