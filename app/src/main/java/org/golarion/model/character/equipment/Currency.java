package org.golarion.model.character.equipment;

import lombok.Getter;

@Getter
public enum Currency
{
    COPPER("Rame", 1),
    SILVER("Argento", 10),
    GOLD("Oro", 100),
    PLATINUM("Platino", 1000);

    private final String displayName;
    private final int copperValue;

    Currency(String displayName, int copperValue)
    {
        this.displayName = displayName;
        this.copperValue = copperValue;
    }
}
