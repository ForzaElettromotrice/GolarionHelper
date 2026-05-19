package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellFilterMatchMode
{
    ANY("Qualsiasi"),
    ALL("Tutti");

    private final String displayName;

    SpellFilterMatchMode(String displayName)
    {
        this.displayName = displayName;
    }
}
