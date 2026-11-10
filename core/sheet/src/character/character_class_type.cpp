#include "golarion/character/character_class_type.hpp"

#include <stdexcept>

namespace golarion
{
    CharacterClassType characterClassFromId(std::string_view id)
    {
        if (id == "barbarian") return CharacterClassType::Barbarian;
        if (id == "bard") return CharacterClassType::Bard;
        if (id == "cleric") return CharacterClassType::Cleric;
        if (id == "druid") return CharacterClassType::Druid;
        if (id == "fighter") return CharacterClassType::Fighter;
        if (id == "rogue") return CharacterClassType::Rogue;
        if (id == "wizard") return CharacterClassType::Wizard;
        if (id == "monk") return CharacterClassType::Monk;
        if (id == "paladin") return CharacterClassType::Paladin;
        if (id == "ranger") return CharacterClassType::Ranger;
        if (id == "sorcerer") return CharacterClassType::Sorcerer;
        if (id == "alchemist") return CharacterClassType::Alchemist;
        if (id == "cavalier") return CharacterClassType::Cavalier;
        if (id == "summoner") return CharacterClassType::Summoner;
        if (id == "witch") return CharacterClassType::Witch;
        if (id == "inquisitor") return CharacterClassType::Inquisitor;
        if (id == "magus") return CharacterClassType::Magus;
        if (id == "shifter") return CharacterClassType::Shifter;
        if (id == "oracle") return CharacterClassType::Oracle;
        if (id == "gunslinger") return CharacterClassType::Gunslinger;
        if (id == "vigilante") return CharacterClassType::Vigilante;
        if (id == "antipaladin") return CharacterClassType::Antipaladin;
        if (id == "ninja") return CharacterClassType::Ninja;
        if (id == "samurai") return CharacterClassType::Samurai;
        if (id == "arcanist") return CharacterClassType::Arcanist;
        if (id == "brawler") return CharacterClassType::Brawler;
        if (id == "hunter") return CharacterClassType::Hunter;
        if (id == "swashbuckler") return CharacterClassType::Swashbuckler;
        if (id == "investigator") return CharacterClassType::Investigator;
        if (id == "bloodrager") return CharacterClassType::Bloodrager;
        if (id == "slayer") return CharacterClassType::Slayer;
        if (id == "warpriest") return CharacterClassType::Warpriest;
        if (id == "skald") return CharacterClassType::Skald;
        if (id == "shaman") return CharacterClassType::Shaman;
        if (id == "kineticist") return CharacterClassType::Kineticist;
        if (id == "medium") return CharacterClassType::Medium;
        if (id == "mesmerist") return CharacterClassType::Mesmerist;
        if (id == "occultist") return CharacterClassType::Occultist;
        if (id == "psychic") return CharacterClassType::Psychic;
        if (id == "spiritualist") return CharacterClassType::Spiritualist;
        if (id == "unchainedBarbarian") return CharacterClassType::UnchainedBarbarian;
        if (id == "unchainedSummoner") return CharacterClassType::UnchainedSummoner;
        if (id == "unchainedRogue") return CharacterClassType::UnchainedRogue;
        if (id == "unchainedMonk") return CharacterClassType::UnchainedMonk;
        if (id == "adept") return CharacterClassType::Adept;
        throw std::invalid_argument("unknown character class ID");
    }

    std::string_view characterClassId(CharacterClassType type)
    {
        switch (type)
        {
            case CharacterClassType::Barbarian: return "barbarian";
            case CharacterClassType::Bard: return "bard";
            case CharacterClassType::Cleric: return "cleric";
            case CharacterClassType::Druid: return "druid";
            case CharacterClassType::Fighter: return "fighter";
            case CharacterClassType::Rogue: return "rogue";
            case CharacterClassType::Wizard: return "wizard";
            case CharacterClassType::Monk: return "monk";
            case CharacterClassType::Paladin: return "paladin";
            case CharacterClassType::Ranger: return "ranger";
            case CharacterClassType::Sorcerer: return "sorcerer";
            case CharacterClassType::Alchemist: return "alchemist";
            case CharacterClassType::Cavalier: return "cavalier";
            case CharacterClassType::Summoner: return "summoner";
            case CharacterClassType::Witch: return "witch";
            case CharacterClassType::Inquisitor: return "inquisitor";
            case CharacterClassType::Magus: return "magus";
            case CharacterClassType::Shifter: return "shifter";
            case CharacterClassType::Oracle: return "oracle";
            case CharacterClassType::Gunslinger: return "gunslinger";
            case CharacterClassType::Vigilante: return "vigilante";
            case CharacterClassType::Antipaladin: return "antipaladin";
            case CharacterClassType::Ninja: return "ninja";
            case CharacterClassType::Samurai: return "samurai";
            case CharacterClassType::Arcanist: return "arcanist";
            case CharacterClassType::Brawler: return "brawler";
            case CharacterClassType::Hunter: return "hunter";
            case CharacterClassType::Swashbuckler: return "swashbuckler";
            case CharacterClassType::Investigator: return "investigator";
            case CharacterClassType::Bloodrager: return "bloodrager";
            case CharacterClassType::Slayer: return "slayer";
            case CharacterClassType::Warpriest: return "warpriest";
            case CharacterClassType::Skald: return "skald";
            case CharacterClassType::Shaman: return "shaman";
            case CharacterClassType::Kineticist: return "kineticist";
            case CharacterClassType::Medium: return "medium";
            case CharacterClassType::Mesmerist: return "mesmerist";
            case CharacterClassType::Occultist: return "occultist";
            case CharacterClassType::Psychic: return "psychic";
            case CharacterClassType::Spiritualist: return "spiritualist";
            case CharacterClassType::UnchainedBarbarian: return "unchainedBarbarian";
            case CharacterClassType::UnchainedSummoner: return "unchainedSummoner";
            case CharacterClassType::UnchainedRogue: return "unchainedRogue";
            case CharacterClassType::UnchainedMonk: return "unchainedMonk";
            case CharacterClassType::Adept: return "adept";
        }
        throw std::invalid_argument("unknown character class type");
    }
}
