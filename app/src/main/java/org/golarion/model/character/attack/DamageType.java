package org.golarion.model.character.attack;

import lombok.Getter;

@Getter
public enum DamageType
{
    BLUDGEONING("Contundente"),
    PIERCING("Perforante"),
    SLASHING("Tagliente"),
    ACID("Acido"),
    COLD("Freddo"),
    ELECTRICITY("Elettricita"),
    FIRE("Fuoco"),
    SONIC("Sonoro"),
    FORCE("Forza"),
    NEGATIVE("Energia Negativa"),
    POSITIVE("Energia Positiva"),
    PRECISION("Precisione"),
    BLEED("Sanguinamento"),
    OTHER("Altro");

    private final String displayName;

    DamageType(String displayName)
    {
        this.displayName = displayName;
    }
}
