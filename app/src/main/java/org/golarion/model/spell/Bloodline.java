package org.golarion.model.spell;

import lombok.Getter;

public enum Bloodline
{
    ABERRANTE("Aberrante"),
    ABISSALE("Abissale"),
    ACQUATICA("Acquatica"),
    ARCANA("Arcana"),
    BOREALE("Boreale"),
    CELESTIALE("Celestiale"),
    DJINNI("Djinni"),
    DRACONICA("Draconica"),
    ECTOPLASMICA("Ectoplasmica"),
    EFREETI("Efreeti"),
    ELEMENTALE("Elementale"),
    ELETTA("Eletta"),
    FATATA("Fatata"),
    IMPERIOSA("Imperiosa (Umano)"),
    INFERNALE("Infernale"),
    MAGISTRALE("Magistrale"),
    MALEDETTA("Maledetta"),
    MARID("Marid"),
    NON_MORTA("Non Morta"),
    OMBRA("Ombra"),
    PARAPSICHICA("Parapsichica"),
    PROTEAN("Protean"),
    RAKSHASA("Rakshasa"),
    SERPENTIFORME("Serpentiforme"),
    SHAITAN("Shaitan"),
    SOGNATRICE("Sognatrice"),
    SOTTERRANEA("Sotterranea"),
    STREGONE_COBOLDO("Stregone Coboldo (Coboldo)"),
    STELLARE("Stellare"),
    TEMPESTOSA("Tempestosa"),
    VERDEGGIANTE("Verdeggiante");

    @Getter
    private final String toString;

    Bloodline(String toString)
    {
        this.toString = toString;
    }
}
