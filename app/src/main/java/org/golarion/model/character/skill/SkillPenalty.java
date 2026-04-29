package org.golarion.model.character.skill;

import lombok.AccessLevel;
import lombok.Getter;
import lombok.Setter;

import java.util.UUID;

@Getter
public class SkillPenalty
{
    private final UUID id;
    private final String source;
    private final int value;
    @Setter(AccessLevel.PACKAGE)
    private boolean enabled;
    private final String description;

    public SkillPenalty(String source, int value)
    {
        this(source, value, true, "");
    }

    public SkillPenalty(String source, int value, boolean enabled, String description)
    {
        this.id = UUID.randomUUID();
        this.source = validateSource(source);
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
