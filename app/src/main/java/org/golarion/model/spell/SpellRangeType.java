package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellRangeType
{
    PERSONAL("Personale"),
    TOUCH("Contatto"),
    CLOSE("Vicino"),
    MEDIUM("Medio"),
    LONG("Lungo"),
    UNLIMITED("Illimitato"),
    OTHER("Altro");

    private final String displayName;

    SpellRangeType(String displayName)
    {
        this.displayName = displayName;
    }
}
