package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellCastingTimeCategory
{
    SWIFT_ACTION("Azione Veloce"),
    IMMEDIATE_ACTION("Azione Immediata"),
    STANDARD_ACTION("Azione Standard"),
    FULL_ROUND_ACTION("Round Completo"),
    ROUND("Round"),
    MINUTE("Minuto"),
    HOUR("Ora"),
    DAY("Giorno"),
    SPECIAL("Speciale");

    private final String displayName;

    SpellCastingTimeCategory(String displayName)
    {
        this.displayName = displayName;
    }
}
