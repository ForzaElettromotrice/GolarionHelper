package org.golarion.model.stats;

import lombok.Getter;

public enum Characteristic
{
    STRENGTH("Strength", "Str"),
    DEXTERITY("Dexterity", "Dex"),
    CONSTITUTION("Constitution", "Con"),
    INTELLIGENCE("Intelligence", "Int"),
    WISDOM("Wisdom", "Wis"),
    CHARISMA("Charisma", "Cha");

    @Getter
    private final String name;
    @Getter
    private final String shortName;

    Characteristic(String name, String shortName)
    {
        this.name = name;
        this.shortName = shortName;
    }
}
