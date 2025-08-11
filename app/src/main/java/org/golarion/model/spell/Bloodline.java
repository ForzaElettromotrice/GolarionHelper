package org.golarion.model.spell;

import lombok.Getter;

public enum Bloodline
{
    ABERRANT("Aberrante"),
    ABYSSAL("Abissale"),
    ACQUATIC("Acquatica"),
    ARCANE("Arcana"),
    BOREAL("Boreale"),
    CELESTIAL("Celestiale"),
    DJINNI("Djinni"),
    DRACONIC("Draconica"),
    ECTOPLASMIC("Ectoplasmica"),
    EFREETI("Efreeti"),
    ELEMENTAL("Elementale"),
    DESTINED("Eletta"),
    FEY("Fatata"),
    IMPERIUS("Imperiosa (Umano)"),
    INFERNAL("Infernale"),
    MAESTRO("Magistrale"),
    ACCURSED("Maledetta"),
    MARID("Marid"),
    UNDEAD("Non Morta"),
    SHADOW("Ombra"),
    PSYCHIC("Parapsichica"),
    PROTEAN("Protean"),
    RAKSHASA("Rakshasa"),
    SERPENTINE("Serpentiforme"),
    SHAITAN("Shaitan"),
    DREAMSPUN("Sognatrice"),
    DEEPEARTH("Sotterranea"),
    KOBOLD("Stregone Coboldo (Coboldo)"),
    STARSOUL("Stellare"),
    STORMBORN("Tempestosa"),
    VERDANT("Verdeggiante");

    @Getter
    private final String toString;

    Bloodline(String toString)
    {
        this.toString = toString;
    }
}
