package org.golarion.model.spell;

import lombok.Getter;

public enum Descriptor
{
    NULL(""),
    ACID("Acido"),
    WATER("Acqua"),
    AIR("Aria"),
    GOOD("Bene"),
    CHAOS("Caotico"),
    LANGUAGE("Dipendenti dal Linguaggio"),
    PAIN("Dolore"),
    ELECTRICITY("Elettricità"),
    EMOTION("Emozione"),
    STRENGTH("Forza"),
    COLD("Freddo"),
    FIRE("Fuoco"),
    MIND("Influenza Mentale"),
    LAWFUL("Legale"),
    LIGHT("Luce"),
    ILLNESS("Malattia"),
    EVIL("Male"),
    CURSE("Maledizione"),
    SHADOW("Ombra"),
    DARKNESS("Oscurità"),
    FEAR("Paura"),
    SOUND("Sonoro"),
    EARTH("Terra"),
    TRICK("Trucco"),
    POISON("Veleno");

    @Getter
    private final String toString;

    Descriptor(String toString)
    {
        this.toString = toString;
    }
}
