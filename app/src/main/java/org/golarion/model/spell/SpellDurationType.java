package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellDurationType
{
    INSTANTANEOUS("Istantaneo"),
    PERMANENT("Permanente"),
    CONCENTRATION("Concentrazione"),
    ROUND("Round"),
    MINUTE("Minuto"),
    HOUR("Ora"),
    DAY("Giorno"),
    OTHER("Altro");

    private final String displayName;

    SpellDurationType(String displayName)
    {
        this.displayName = displayName;
    }
}
