package org.golarion.model.spell;

import com.google.gson.JsonObject;
import lombok.Getter;

import java.util.Map;

public enum GClass
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
    SHAMAN("Sciamano");

    public static Map<GClass, Integer> parseClassLevels(JsonObject classLevels)
    {
        return classLevels.entrySet().stream()
                .collect(java.util.stream.Collectors.toMap(
                        e -> GClass.valueOf(e.getKey().toUpperCase()),
                        e -> e.getValue().getAsInt()));
    }

    @Getter
    private final String toString;

    GClass(String toString)
    {
        this.toString = toString;
    }
}
