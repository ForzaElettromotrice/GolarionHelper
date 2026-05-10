package org.golarion.model.character.combat;

import lombok.Getter;

@Getter
public enum WeaponType
{
    LIGHT("Leggera"),
    ONE_HANDED("Una Mano"),
    TWO_HANDED("Due Mani"),
    RANGED("A Distanza");

    private final String displayName;

    WeaponType(String displayName)
    {
        this.displayName = displayName;
    }
}
