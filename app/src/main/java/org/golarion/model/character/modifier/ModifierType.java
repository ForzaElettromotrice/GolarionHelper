package org.golarion.model.character.modifier;

import lombok.Getter;

public enum ModifierType
{
    BONUS("Bonus"),
    PENALTY("Malus");

    @Getter
    private final String name;

    private ModifierType(String name)
    {
        this.name = name;
    }
}
