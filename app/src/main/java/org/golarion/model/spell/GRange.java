package org.golarion.model.spell;

import lombok.Getter;

public enum GRange
{
    PERSONALE("Personale"),
    CONTATTO("Contatto"),
    VICINO("Vicino"),
    MEDIO("Medio"),
    LUNGO("Lungo"),
    ILLIMITATO("Illimitato");

    @Getter
    private final String toString;

    GRange(String toString)
    {
        this.toString = toString;
    }
}
