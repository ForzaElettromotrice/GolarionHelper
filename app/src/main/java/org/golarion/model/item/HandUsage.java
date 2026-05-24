package org.golarion.model.item;

import lombok.Getter;

@Getter
public enum HandUsage
{
    NONE("Nessuna"),
    ONE_HAND("Una Mano"),
    TWO_HANDS("Due Mani");

    private final String displayName;

    HandUsage(String displayName)
    {
        this.displayName = displayName;
    }
}
