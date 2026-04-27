package org.golarion.model.character;

import lombok.Getter;

@Getter
public enum Ability
{
    STRENGTH("Forza"),
    DEXTERITY("Destrezza"),
    CONSTITUTION("Costituzione"),
    INTELLIGENCE("Intelligenza"),
    WISDOM("Saggezza"),
    CHARISMA("Carisma");

    private final String displayName;

    Ability(String displayName)
    {
        this.displayName = displayName;
    }
}
