package org.golarion.model.spell;

import lombok.Getter;

public enum SavingThrow
{
    NESSUNO("Nessuno"),
    TEMPRA("Tempra"),
    RIFLESSI("Riflessi"),
    VOLONTA("Volontà");

    @Getter
    private final String toString;

    SavingThrow(String toString)
    {
        this.toString = toString;
    }
}
