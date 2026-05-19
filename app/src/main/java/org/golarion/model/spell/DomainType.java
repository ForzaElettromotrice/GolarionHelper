package org.golarion.model.spell;

import lombok.Getter;

import java.util.Set;

@Getter
public enum DomainType implements DomainSpellListType
{
    AIR("Aria"),
    ANIMAL("Animale"),
    ARTIFICE("Artificio"),
    CHAOS("Caos"),
    CHARM("Charme"),
    COMMUNITY("Comunità"),
    DARKNESS("Oscurità"),
    DEATH("Morte"),
    DESTRUCTION("Distruzione"),
    EARTH("Terra"),
    EVIL("Male"),
    FIRE("Fuoco"),
    GLORY("Gloria"),
    GOOD("Bene"),
    HEALING("Guarigione"),
    KNOWLEDGE("Conoscenza"),
    LAW("Legge"),
    LIBERATION("Liberazione"),
    LUCK("Fortuna"),
    MADNESS("Follia"),
    MAGIC("Magia"),
    NOBILITY("Nobiltà"),
    PLANT("Vegetale"),
    PROTECTION("Protezione"),
    REPTILE("Rettili"),
    REPOSE("Riposo"),
    RUNE("Rune"),
    STRENGTH("Forza"),
    SUN("Sole"),
    TRAVEL("Viaggio"),
    TRICKERY("Inganno"),
    WAR("Guerra"),
    WATER("Acqua"),
    WEATHER("Tempo Atmosferico"),
    VOID("Vuoto");

    private final String displayName;

    DomainType(String displayName)
    {
        this.displayName = displayName;
    }

    @Override
    public Set<DomainType> getDomains()
    {
        return Set.of(this);
    }
}
