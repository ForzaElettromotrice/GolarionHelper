package org.golarion.model.character.attack;

import lombok.Getter;

@Getter
public enum AttackType
{
    MELEE("Mischia"),
    RANGED("Distanza"),
    THROWN("Lancio"),
    NATURAL("Naturale"),
    UNARMED("Senz'armi");

    private final String displayName;

    AttackType(String displayName)
    {
        this.displayName = displayName;
    }
}
