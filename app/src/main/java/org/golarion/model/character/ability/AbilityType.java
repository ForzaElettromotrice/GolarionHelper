package org.golarion.model.character.ability;

import lombok.Getter;

@Getter
public enum AbilityType
{
    STRENGTH("Forza"),
    DEXTERITY("Destrezza"),
    CONSTITUTION("Costituzione"),
    INTELLIGENCE("Intelligenza"),
    WISDOM("Saggezza"),
    CHARISMA("Carisma");

    private final String displayName;

    AbilityType(String displayName)
    {
        this.displayName = displayName;
    }
}
