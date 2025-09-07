package org.golarion.model.spell;

import lombok.Getter;

public enum School
{
    ABIURAZIONE("Abiurazione"),
    AMMALIAMENTO("Ammaliamento"),
    DIVINAZIONE("Divinazione"),
    EVOCAZIONE("Evocazione"),
    ILLUSIONE("Illusione"),
    INVOCAZIONE("Invocazione"),
    NECROMANZIA("Necromanzia"),
    TRASMUTAZIONE("Trasmutazione"),
    UNIVERSALE("Universale");

    @Getter
    private final String toString;

    School(String toString)
    {
        this.toString = toString;
    }
}
