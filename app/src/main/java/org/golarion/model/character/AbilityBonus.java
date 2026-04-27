package org.golarion.model.character;

import lombok.Getter;
import lombok.Setter;
import lombok.AccessLevel;

import java.util.UUID;

@Getter
public class AbilityBonus
{
    private final UUID id;
    private final String source;
    private final BonusType bonusType;
    private final int value;
    @Setter(AccessLevel.PACKAGE)
    private boolean enabled;
    private final String description;

    public AbilityBonus(String source, BonusType bonusType, int value)
    {
        this(source, bonusType, value, true, "");
    }

    public AbilityBonus(String source, BonusType bonusType, int value, boolean enabled, String description)
    {
        this.id = UUID.randomUUID();
        this.source = validateSource(source);
        this.bonusType = validateBonusType(bonusType);
        this.value = validateValue(value);
        this.enabled = enabled;
        this.description = normalize(description);
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
}
