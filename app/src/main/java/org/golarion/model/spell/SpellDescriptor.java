package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellDescriptor
{
    ACID("Acido"),
    AIR("Aria"),
    CHAOTIC("Caotico"),
    COLD("Freddo"),
    CURSE("Maledizione"),
    DARKNESS("Oscurità"),
    DEATH("Morte"),
    DISEASE("Malattia"),
    EARTH("Terra"),
    ELECTRICITY("Elettricità"),
    EMOTION("Emozione"),
    EVIL("Male"),
    FEAR("Paura"),
    FIRE("Fuoco"),
    FORCE("Forza"),
    GOOD("Bene"),
    LANGUAGE_DEPENDENT("Dipendente dal Linguaggio"),
    LAWFUL("Legale"),
    LIGHT("Luce"),
    MIND_AFFECTING("Influenza Mentale"),
    PAIN("Dolore"),
    POISON("Veleno"),
    SHADOW("Ombra"),
    SONIC("Sonoro"),
    WATER("Acqua");

    private final String displayName;

    SpellDescriptor(String displayName)
    {
        this.displayName = displayName;
    }
}
