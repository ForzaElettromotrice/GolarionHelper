package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellResistanceType
{
    YES("Sì"),
    NO("No"),
    SEE_TEXT("Vedi Testo");

    private final String displayName;

    SpellResistanceType(String displayName)
    {
        this.displayName = displayName;
    }
}
