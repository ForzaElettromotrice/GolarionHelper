package org.golarion.model.spell;

import lombok.Getter;

public enum GDuration
{
    GIORNI("Giorni"),
    ORE("Ore"),
    MINUTI("Minuti"),
    ROUND("Round"),
    ISTANTANEO("Istantaneo");

    @Getter
    private final String name;

    GDuration(String name)
    {
        this.name = name;
    }
}
