package org.golarion.model.character.combat;

import lombok.Getter;

@Getter
public enum WeaponCategory
{
    SIMPLE("Semplice"),
    MARTIAL("Da Guerra"),
    EXOTIC("Esotica"),
    OTHER("Altro");

    private final String displayName;

    WeaponCategory(String displayName)
    {
        this.displayName = displayName;
    }
}
