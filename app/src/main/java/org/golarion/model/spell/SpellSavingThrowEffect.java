package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellSavingThrowEffect
{
    NEGATES("Nega"),
    PARTIAL("Parziale"),
    HALF("Dimezza"),
    SEE_TEXT("Vedi Testo"),
    HARMLESS("Innocuo"),
    OBJECT("Oggetto"),
    DISBELIEF("Dubita");

    private final String displayName;

    SpellSavingThrowEffect(String displayName)
    {
        this.displayName = displayName;
    }
}
