package org.golarion.model.spell;

import lombok.Getter;

public enum Descriptor
{
    ACIDO("Acido"),
    ACQUA("Acqua"),
    ARIA("Aria"),
    BENE("Bene"),
    CAOS("Caos"),
    DIPENDENTE_DAL_LINGUAGGIO("Dipendente dal Linguaggio"),
    DOLORE("Dolore"),
    ELETTRICITA("Elettricità"),
    EMOZIONE("Emozione"),
    FORZA("Forza"),
    FREDDO("Freddo"),
    FUOCO("Fuoco"),
    INFLUENZA_MENTALE("Influenza Mentale"),
    LEGALE("Legale"),
    LUCE("Luce"),
    MALATTIA("Malattia"),
    MALE("Male"),
    MALEDIZIONE("Maledizione"),
    MORTE("Morte"),
    OMBRA("Ombra"),
    OSCURITA("Oscurità"),
    PAURA("Paura"),
    SONORO("Sonoro"),
    TERRA("Terra"),
    TRUCCO("Trucco"),
    VELENO("Veleno");

    @Getter
    private final String name;

    Descriptor(String name)
    {
        this.name = name;
    }
}
