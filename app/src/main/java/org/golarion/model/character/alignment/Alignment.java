package org.golarion.model.character.alignment;

import lombok.Getter;

@Getter
public enum Alignment
{
    LAWFUL_GOOD("Legale Buono"),
    NEUTRAL_GOOD("Neutrale Buono"),
    CHAOTIC_GOOD("Caotico Buono"),
    LAWFUL_NEUTRAL("Legale Neutrale"),
    TRUE_NEUTRAL("Neutrale Puro"),
    CHAOTIC_NEUTRAL("Caotico Neutrale"),
    LAWFUL_EVIL("Legale Malvagio"),
    NEUTRAL_EVIL("Neutrale Malvagio"),
    CHAOTIC_EVIL("Caotico Malvagio");

    private final String displayName;

    Alignment(String displayName)
    {
        this.displayName = displayName;
    }
}
