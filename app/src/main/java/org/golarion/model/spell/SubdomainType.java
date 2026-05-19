package org.golarion.model.spell;

import lombok.Getter;

import java.util.EnumSet;
import java.util.Set;

@Getter
public enum SubdomainType implements DomainSpellListType
{
    AGATHION("Agathion", EnumSet.of(DomainType.GOOD)),
    ALIENATION("Alienazione", EnumSet.of(DomainType.MADNESS)),
    ANCESTORS("Antenati", EnumSet.of(DomainType.REPOSE)),
    ARCANE("Arcano", EnumSet.of(DomainType.MAGIC)),
    ARCHON("Arconti", EnumSet.of(DomainType.GOOD, DomainType.LAW)),
    ASH("Cenere", EnumSet.of(DomainType.FIRE)),
    ASSASSINATION("Assassinio", EnumSet.of(DomainType.DEATH)),
    AZATA("Azata", EnumSet.of(DomainType.CHAOS, DomainType.GOOD)),
    BLOOD("Sangue", EnumSet.of(DomainType.WAR)),
    CATASTROPHE("Catastrofe", EnumSet.of(DomainType.DESTRUCTION)),
    CAVES("Caverne", EnumSet.of(DomainType.EARTH)),
    CLOUDS("Nubi", EnumSet.of(DomainType.AIR)),
    COMMERCE("Commercio", EnumSet.of(DomainType.TRAVEL)),
    CONSTRUCT("Costrutto", EnumSet.of(DomainType.ARTIFICE)),
    CURSE("Maledizione", EnumSet.of(DomainType.LUCK)),
    DARK_TAPESTRY("Arazzo Oscuro", EnumSet.of(DomainType.VOID)),
    DAEMON("Daemon", EnumSet.of(DomainType.EVIL)),
    DAY("Giorno", EnumSet.of(DomainType.SUN)),
    DECAY("Decomposizione", EnumSet.of(DomainType.PLANT)),
    DEFENSE("Difesa", EnumSet.of(DomainType.PROTECTION)),
    DEMON("Demoni", EnumSet.of(DomainType.CHAOS, DomainType.EVIL)),
    DEVIL("Diavoli", EnumSet.of(DomainType.EVIL, DomainType.LAW)),
    DIVINE("Divino", EnumSet.of(DomainType.MAGIC)),
    EXPLORATION("Esplorazione", EnumSet.of(DomainType.TRAVEL)),
    FAMILY("Famiglia", EnumSet.of(DomainType.COMMUNITY)),
    FATE("Fato", EnumSet.of(DomainType.LUCK)),
    FEATHER("Piuma", EnumSet.of(DomainType.ANIMAL)),
    FEROCITY("Ferocia", EnumSet.of(DomainType.STRENGTH)),
    FUR("Pelliccia", EnumSet.of(DomainType.ANIMAL)),
    GROWTH("Crescita", EnumSet.of(DomainType.PLANT)),
    HEALING("Risanamento", EnumSet.of(DomainType.HEALING)),
    HEROISM("Eroismo", EnumSet.of(DomainType.GLORY)),
    HOME("Casa", EnumSet.of(DomainType.COMMUNITY)),
    HONOR("Onore", EnumSet.of(DomainType.GLORY)),
    ICE("Ghiaccio", EnumSet.of(DomainType.WATER)),
    INEVITABLES("Inevitabili", EnumSet.of(DomainType.LAW)),
    INTERDICTIONS("Interdizioni", EnumSet.of(DomainType.RUNE)),
    ISOLATION("Isolamento", EnumSet.of(DomainType.VOID)),
    LANGUAGE("Linguaggio", EnumSet.of(DomainType.RUNE)),
    LEADERSHIP("Autorità", EnumSet.of(DomainType.NOBILITY)),
    LIBERATION("Liberazione", EnumSet.of(DomainType.LIBERATION)),
    LIGHT("Luce", EnumSet.of(DomainType.SUN)),
    LOSS("Perdita", EnumSet.of(DomainType.DARKNESS)),
    LOYALTY("Lealtà", EnumSet.of(DomainType.LAW)),
    LOVE("Amore", EnumSet.of(DomainType.CHARM)),
    LUST("Lussuria", EnumSet.of(DomainType.CHARM)),
    MARTYRDOM("Martirio", EnumSet.of(DomainType.NOBILITY)),
    MEMORY("Memoria", EnumSet.of(DomainType.KNOWLEDGE)),
    METAL("Metallo", EnumSet.of(DomainType.EARTH)),
    MOON("Luna", EnumSet.of(DomainType.DARKNESS)),
    NIGHT("Notte", EnumSet.of(DomainType.DARKNESS)),
    NIGHTMARE("Incubo", EnumSet.of(DomainType.MADNESS)),
    OCEANS("Oceani", EnumSet.of(DomainType.WATER)),
    PROTEAN("Protean", EnumSet.of(DomainType.CHAOS)),
    PURITY("Purezza", EnumSet.of(DomainType.PROTECTION)),
    RAGE("Ira", EnumSet.of(DomainType.DESTRUCTION)),
    RESOLUTION("Risolutezza", EnumSet.of(DomainType.STRENGTH)),
    RESURRECTION("Resurrezione", EnumSet.of(DomainType.HEALING)),
    REVOLUTION("Rivoluzione", EnumSet.of(DomainType.LIBERATION)),
    RIVERS("Fiumi", EnumSet.of(DomainType.WATER)),
    SEASONS("Stagioni", EnumSet.of(DomainType.WEATHER)),
    SMOKE("Fumo", EnumSet.of(DomainType.FIRE)),
    SOULS("Anime", EnumSet.of(DomainType.REPOSE)),
    STARS("Stelle", EnumSet.of(DomainType.VOID)),
    STORMS("Tempeste", EnumSet.of(DomainType.WEATHER)),
    SUBTERFUGE("Sotterfugio", EnumSet.of(DomainType.TRICKERY)),
    TACTICS("Tattiche", EnumSet.of(DomainType.WAR)),
    THEFT("Furto", EnumSet.of(DomainType.TRICKERY)),
    THOUGHT("Pensiero", EnumSet.of(DomainType.KNOWLEDGE)),
    UNDEATH("Non Morte", EnumSet.of(DomainType.DEATH)),
    WHIMSY("Capriccio", EnumSet.of(DomainType.CHAOS)),
    WIND("Vento", EnumSet.of(DomainType.AIR));

    private final String displayName;
    private final Set<DomainType> associatedDomains;

    SubdomainType(String displayName, Set<DomainType> associatedDomains)
    {
        this.displayName = displayName;
        this.associatedDomains = Set.copyOf(associatedDomains);
    }

    @Override
    public Set<DomainType> getDomains()
    {
        return associatedDomains;
    }
}
