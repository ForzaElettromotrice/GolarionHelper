package org.golarion.model.character.characterclass;

import lombok.Getter;

@Getter
public enum CharacterClassType
{
    BARBARIAN("Barbaro"),
    BARD("Bardo"),
    CLERIC("Chierico"),
    DRUID("Druido"),
    FIGHTER("Guerriero"),
    ROGUE("Ladro"),
    WIZARD("Mago"),
    MONK("Monaco"),
    PALADIN("Paladino"),
    RANGER("Ranger"),
    SORCERER("Stregone"),
    ALCHEMIST("Alchimista"),
    CAVALIER("Cavaliere"),
    SUMMONER("Convocatore"),
    WITCH("Fattucchiere"),
    INQUISITOR("Inquisitore"),
    MAGUS("Magus"),
    SHIFTER("Morfico"),
    ORACLE("Oracolo"),
    GUNSLINGER("Pistolero"),
    VIGILANTE("Vigilante"),
    ANTIPALADIN("Antipaladino"),
    NINJA("Ninja"),
    SAMURAI("Samurai"),
    ARCANIST("Arcanista"),
    BRAWLER("Attaccabrighe"),
    HUNTER("Cacciatore"),
    SWASHBUCKLER("Intrepido"),
    INVESTIGATOR("Investigatore"),
    BLOODRAGER("Iracondo di Stirpe"),
    SLAYER("Predatore"),
    WARPRIEST("Sacerdote Guerriero"),
    SKALD("Scaldo"),
    SHAMAN("Sciamano"),
    KINETICIST("Cineta"),
    MEDIUM("Medium"),
    MESMERIST("Mesmerista"),
    OCCULTIST("Occultista"),
    PSYCHIC("Parapsichico"),
    SPIRITUALIST("Spiritista"),
    UNCHAINED_BARBARIAN("Barbaro Rivisitato"),
    UNCHAINED_SUMMONER("Convocatore Rivisitato"),
    UNCHAINED_ROGUE("Ladro Rivisitato"),
    UNCHAINED_MONK("Monaco Rivisitato");

    private final String displayName;

    CharacterClassType(String displayName)
    {
        this.displayName = displayName;
    }
}
