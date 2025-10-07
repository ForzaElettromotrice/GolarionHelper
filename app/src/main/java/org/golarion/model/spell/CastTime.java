package org.golarion.model.spell;

import lombok.Getter;

public enum CastTime
{
    STANDARD("Standard"),
    ROUND("Round"),
    VELOCE("Veloce"),
    IMMEDIATA("Immediata");

    @Getter
    private final String name;

    CastTime(String name)
    {
        this.name = name;
    }
}
