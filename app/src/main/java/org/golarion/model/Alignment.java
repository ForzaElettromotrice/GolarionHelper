package org.golarion.model;

import lombok.Getter;

public enum Alignment
{
    LAWFUL_GOOD("Lawful Good"),
    NEUTRAL_GOOD("Neutral Good"),
    CHAOTIC_GOOD("Chaotic Good"),
    LAWFUL_NEUTRAL("Lawful Neutral"),
    TRUE_NEUTRAL("True Neutral"),
    CHAOTIC_NEUTRAL("Chaotic Neutral"),
    LAWFUL_EVIL("Lawful Evil"),
    NEUTRAL_EVIL("Neutral Evil"),
    CHAOTIC_EVIL("Chaotic Evil");

    @Getter
    private final String toString;

    Alignment(String toString)
    {
        this.toString = toString;
    }
}
