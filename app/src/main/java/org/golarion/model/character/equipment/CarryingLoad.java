package org.golarion.model.character.equipment;

import lombok.Getter;

@Getter
public enum CarryingLoad
{
    LIGHT("Leggero"),
    MEDIUM("Medio"),
    HEAVY("Pesante"),
    OVERLOADED("Sovraccarico");

    private final String displayName;

    CarryingLoad(String displayName)
    {
        this.displayName = displayName;
    }
}
