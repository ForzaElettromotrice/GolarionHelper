package org.golarion.model.spell;

import lombok.Getter;

public enum DomainList implements Domain
{
    ACQUA("Acqua"),
    ANIMALE("Animale"),
    ARIA("Aria"),
    ARTIFICIO("Artificio"),
    BENE("Bene"),
    CAOS("Caos"),
    CHARME("Charme"),
    COMUNITA("Comunità"),
    CONOSCENZA("Conoscenza"),
    DISTRUZIONE("Distruzione"),
    FOLLIA("Follia"),
    FORTUNA("Fortuna"),
    FORZA("Forza"),
    FUOCO("Fuoco"),
    GLORIA("Gloria"),
    GUARIGIONE("Guarigione"),
    GUERRA("Guerra"),
    INGANNO("Inganno"),
    LEGGE("Legge"),
    LIBERTA("Libertà"),
    MAGIA("Magia"),
    MALE("Male"),
    MORTE("Morte"),
    NOBILTA("Nobiltà"),
    OSCURITA("Oscurità"),
    PROTEZIONE("Protezione"),
    RETTILI("Rettili"),
    RIPOSO("Riposo"),
    RUNE("Rune"),
    SOLE("Sole"),
    TEMPO_ATMOSFERICO("Tempo Atmosferico"),
    TERRA("Terra"),
    VEGETALE("Vegetale"),
    VIAGGIO("Viaggio"),
    VUOTO("Vuoto");

    @Getter
    private final String name;

    DomainList(String name)
    {
        this.name = name;
    }
}
