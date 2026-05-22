package org.golarion.model.character.attack;

import lombok.NonNull;

import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public record DamageDice(
        int diceCount,
        int dieSize
)
{
    private static final Pattern DAMAGE_DICE_PATTERN = Pattern.compile("^(\\d+)\\s*d\\s*(\\d+)$", Pattern.CASE_INSENSITIVE);

    public static DamageDice parse(@NonNull String value)
    {
        Matcher matcher = DAMAGE_DICE_PATTERN.matcher(value.trim().toLowerCase(Locale.ROOT));
        if (!matcher.matches())
        {
            throw new IllegalArgumentException("damage must be in XdY format");
        }

        return new DamageDice(
                Integer.parseInt(matcher.group(1)),
                Integer.parseInt(matcher.group(2))
        );
    }

    public DamageDice
    {
        if (diceCount <= 0)
        {
            throw new IllegalArgumentException("diceCount must be greater than 0");
        }
        if (dieSize <= 0)
        {
            throw new IllegalArgumentException("dieSize must be greater than 0");
        }
    }

    public DamageDice multiply(int multiplier)
    {
        if (multiplier <= 0)
        {
            throw new IllegalArgumentException("multiplier must be greater than 0");
        }

        return new DamageDice(diceCount * multiplier, dieSize);
    }

    @Override
    @NonNull
    public String toString()
    {
        return diceCount + "d" + dieSize;
    }
}
