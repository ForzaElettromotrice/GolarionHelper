package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellSchool
{
    ABJURATION("Abiurazione"),
    CONJURATION("Evocazione"),
    DIVINATION("Divinazione"),
    ENCHANTMENT("Ammaliamento"),
    EVOCATION("Invocazione"),
    ILLUSION("Illusione"),
    NECROMANCY("Necromanzia"),
    TRANSMUTATION("Trasmutazione"),
    UNIVERSAL("Universale");

    private final String displayName;

    SpellSchool(String displayName)
    {
        this.displayName = displayName;
    }
}
