package org.golarion.model.spell;

import lombok.Getter;

public enum GClass
{
    BARBARO("Barbaro"),
    BARDO("Bardo"),
    CHIERICO("Chierico"),
    DRUIDO("Druido"),
    GUERRIERO("Guerriero"),
    LADRO("Ladro"),
    MAGO("Mago"),
    MONACO("Monaco"),
    PALADINO("Paladino"),
    RANGER("Ranger"),
    STREGONE("Stregone"),
    ALCHIMISTA("Alchimista"),
    CAVALIERE("Cavaliere"),
    CONVOCATORE("Convocatore"),
    FATTUCCHIERE("Fattucchiere"),
    INQUISITORE("Inquisitore"),
    MAGUS("Magus"),
    MORFICO("Morfico"),
    ORACOLO("Oracolo"),
    PISTOLERO("Pistolero"),
    VIGILANTE("Vigilante"),
    ANTIPALADINO("Antipaladino"),
    NINJA("Ninja"),
    SAMURAI("Samurai"),
    ARCANISTA("Arcanista"),
    ATTACCABRIGHE("Attaccabrighe"),
    CACCIATORE("Cacciatore"),
    INTREPIDO("Intrepido"),
    INVESTIGATORE("Investigatore"),
    IRACONDO_DI_STIRPE("Iracondo di Stirpe"),
    PREDATORE("Predatore"),
    SACERDOTE_GUERRIERO("Sacerdote Guerriero"),
    SCALDO("Scaldo"),
    SCIAMANO("Sciamano"),
    CINETA("Cineta"),
    MEDIUM("Medium"),
    MESMERISTA("Mesmerista"),
    OCCULTISTA("Occultista"),
    PARAPSICHICO("Parapsichico"),
    SPIRITISTA("Spiritista"),
    BARBARO_RIVISITATO("Barbaro Rivisitato"),
    CONVOCATORE_RIVISITATO("Convocatore Rivisitato"),
    LADRO_RIVISITATO("Ladro Rivisitato"),
    MONACO_RIVISITATO("Monaco Rivisitato");

    @Getter
    private final String toString;

    GClass(String toString)
    {
        this.toString = toString;
    }
}
