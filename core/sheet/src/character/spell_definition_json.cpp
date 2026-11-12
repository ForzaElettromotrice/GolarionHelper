#include "golarion/character/spell_definition_json.hpp"

#include "golarion/util/string_utils.hpp"
#include "golarion/effect/effect_json.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;
    using namespace golarion;

    template<typename Enum> struct EnumNames;

    template<> struct EnumNames<SpellSchool>
    {
        static constexpr std::array values{
            std::pair{std::string_view("ABJURATION"), SpellSchool::Abjuration},
            std::pair{std::string_view("CONJURATION"), SpellSchool::Conjuration},
            std::pair{std::string_view("DIVINATION"), SpellSchool::Divination},
            std::pair{std::string_view("ENCHANTMENT"), SpellSchool::Enchantment},
            std::pair{std::string_view("EVOCATION"), SpellSchool::Evocation},
            std::pair{std::string_view("ILLUSION"), SpellSchool::Illusion},
            std::pair{std::string_view("NECROMANCY"), SpellSchool::Necromancy},
            std::pair{std::string_view("TRANSMUTATION"), SpellSchool::Transmutation},
            std::pair{std::string_view("UNIVERSAL"), SpellSchool::Universal}
        };
    };

    template<> struct EnumNames<SpellSubschool>
    {
        static constexpr std::array values{
            std::pair{std::string_view("CALLING"), SpellSubschool::Calling},
            std::pair{std::string_view("CHARM"), SpellSubschool::Charm},
            std::pair{std::string_view("COMPULSION"), SpellSubschool::Compulsion},
            std::pair{std::string_view("CREATION"), SpellSubschool::Creation},
            std::pair{std::string_view("FIGMENT"), SpellSubschool::Figment},
            std::pair{std::string_view("GLAMER"), SpellSubschool::Glamer},
            std::pair{std::string_view("HEALING"), SpellSubschool::Healing},
            std::pair{std::string_view("PATTERN"), SpellSubschool::Pattern},
            std::pair{std::string_view("PHANTASM"), SpellSubschool::Phantasm},
            std::pair{std::string_view("POLYMORPH"), SpellSubschool::Polymorph},
            std::pair{std::string_view("SCRYING"), SpellSubschool::Scrying},
            std::pair{std::string_view("SHADOW"), SpellSubschool::Shadow},
            std::pair{std::string_view("SUMMONING"), SpellSubschool::Summoning},
            std::pair{std::string_view("TELEPORTATION"), SpellSubschool::Teleportation}
        };
    };

    template<> struct EnumNames<SpellDescriptor>
    {
        static constexpr std::array values{
            std::pair{std::string_view("ACID"), SpellDescriptor::Acid},
            std::pair{std::string_view("AIR"), SpellDescriptor::Air},
            std::pair{std::string_view("CHAOTIC"), SpellDescriptor::Chaotic},
            std::pair{std::string_view("COLD"), SpellDescriptor::Cold},
            std::pair{std::string_view("CURSE"), SpellDescriptor::Curse},
            std::pair{std::string_view("DARKNESS"), SpellDescriptor::Darkness},
            std::pair{std::string_view("DEATH"), SpellDescriptor::Death},
            std::pair{std::string_view("DISEASE"), SpellDescriptor::Disease},
            std::pair{std::string_view("EARTH"), SpellDescriptor::Earth},
            std::pair{std::string_view("ELECTRICITY"), SpellDescriptor::Electricity},
            std::pair{std::string_view("EMOTION"), SpellDescriptor::Emotion},
            std::pair{std::string_view("EVIL"), SpellDescriptor::Evil},
            std::pair{std::string_view("FEAR"), SpellDescriptor::Fear},
            std::pair{std::string_view("FIRE"), SpellDescriptor::Fire},
            std::pair{std::string_view("FORCE"), SpellDescriptor::Force},
            std::pair{std::string_view("GOOD"), SpellDescriptor::Good},
            std::pair{std::string_view("LANGUAGE_DEPENDENT"), SpellDescriptor::LanguageDependent},
            std::pair{std::string_view("LAWFUL"), SpellDescriptor::Lawful},
            std::pair{std::string_view("LIGHT"), SpellDescriptor::Light},
            std::pair{std::string_view("MIND_AFFECTING"), SpellDescriptor::MindAffecting},
            std::pair{std::string_view("PAIN"), SpellDescriptor::Pain},
            std::pair{std::string_view("POISON"), SpellDescriptor::Poison},
            std::pair{std::string_view("SHADOW"), SpellDescriptor::Shadow},
            std::pair{std::string_view("SONIC"), SpellDescriptor::Sonic},
            std::pair{std::string_view("WATER"), SpellDescriptor::Water}
        };
    };

    template<> struct EnumNames<SpellCastingTimeCategory>
    {
        static constexpr std::array values{
            std::pair{std::string_view("SWIFT_ACTION"), SpellCastingTimeCategory::SwiftAction},
            std::pair{std::string_view("IMMEDIATE_ACTION"), SpellCastingTimeCategory::ImmediateAction},
            std::pair{std::string_view("STANDARD_ACTION"), SpellCastingTimeCategory::StandardAction},
            std::pair{std::string_view("FULL_ROUND_ACTION"), SpellCastingTimeCategory::FullRoundAction},
            std::pair{std::string_view("ROUND"), SpellCastingTimeCategory::Round},
            std::pair{std::string_view("MINUTE"), SpellCastingTimeCategory::Minute},
            std::pair{std::string_view("HOUR"), SpellCastingTimeCategory::Hour},
            std::pair{std::string_view("DAY"), SpellCastingTimeCategory::Day},
            std::pair{std::string_view("SPECIAL"), SpellCastingTimeCategory::Special}
        };
    };

    template<> struct EnumNames<SpellComponentType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("VERBAL"), SpellComponentType::Verbal},
            std::pair{std::string_view("SOMATIC"), SpellComponentType::Somatic},
            std::pair{std::string_view("MATERIAL"), SpellComponentType::Material},
            std::pair{std::string_view("FOCUS"), SpellComponentType::Focus},
            std::pair{std::string_view("DIVINE_FOCUS"), SpellComponentType::DivineFocus},
            std::pair{std::string_view("MATERIAL_OR_DIVINE_FOCUS"), SpellComponentType::MaterialOrDivineFocus},
            std::pair{std::string_view("FOCUS_OR_DIVINE_FOCUS"), SpellComponentType::FocusOrDivineFocus}
        };
    };

    template<> struct EnumNames<SpellRangeType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("PERSONAL"), SpellRangeType::Personal},
            std::pair{std::string_view("TOUCH"), SpellRangeType::Touch},
            std::pair{std::string_view("CLOSE"), SpellRangeType::Close},
            std::pair{std::string_view("MEDIUM"), SpellRangeType::Medium},
            std::pair{std::string_view("LONG"), SpellRangeType::Long},
            std::pair{std::string_view("UNLIMITED"), SpellRangeType::Unlimited},
            std::pair{std::string_view("OTHER"), SpellRangeType::Other}
        };
    };

    template<> struct EnumNames<SpellDurationType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("INSTANTANEOUS"), SpellDurationType::Instantaneous},
            std::pair{std::string_view("PERMANENT"), SpellDurationType::Permanent},
            std::pair{std::string_view("CONCENTRATION"), SpellDurationType::Concentration},
            std::pair{std::string_view("ROUND"), SpellDurationType::Round},
            std::pair{std::string_view("MINUTE"), SpellDurationType::Minute},
            std::pair{std::string_view("HOUR"), SpellDurationType::Hour},
            std::pair{std::string_view("DAY"), SpellDurationType::Day},
            std::pair{std::string_view("OTHER"), SpellDurationType::Other}
        };
    };

    template<> struct EnumNames<SpellResistanceType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("YES"), SpellResistanceType::Yes},
            std::pair{std::string_view("NO"), SpellResistanceType::No},
            std::pair{std::string_view("SEE_TEXT"), SpellResistanceType::SeeText}
        };
    };

    template<> struct EnumNames<SpellSavingThrowEffect>
    {
        static constexpr std::array values{
            std::pair{std::string_view("NEGATES"), SpellSavingThrowEffect::Negates},
            std::pair{std::string_view("PARTIAL"), SpellSavingThrowEffect::Partial},
            std::pair{std::string_view("HALF"), SpellSavingThrowEffect::Half},
            std::pair{std::string_view("SEE_TEXT"), SpellSavingThrowEffect::SeeText},
            std::pair{std::string_view("HARMLESS"), SpellSavingThrowEffect::Harmless},
            std::pair{std::string_view("OBJECT"), SpellSavingThrowEffect::Object},
            std::pair{std::string_view("DISBELIEF"), SpellSavingThrowEffect::Disbelief}
        };
    };

    template<> struct EnumNames<DomainType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("AIR"), DomainType::Air},
            std::pair{std::string_view("ANIMAL"), DomainType::Animal},
            std::pair{std::string_view("ARTIFICE"), DomainType::Artifice},
            std::pair{std::string_view("CHAOS"), DomainType::Chaos},
            std::pair{std::string_view("CHARM"), DomainType::Charm},
            std::pair{std::string_view("COMMUNITY"), DomainType::Community},
            std::pair{std::string_view("DARKNESS"), DomainType::Darkness},
            std::pair{std::string_view("DEATH"), DomainType::Death},
            std::pair{std::string_view("DESTRUCTION"), DomainType::Destruction},
            std::pair{std::string_view("EARTH"), DomainType::Earth},
            std::pair{std::string_view("EVIL"), DomainType::Evil},
            std::pair{std::string_view("FIRE"), DomainType::Fire},
            std::pair{std::string_view("GLORY"), DomainType::Glory},
            std::pair{std::string_view("GOOD"), DomainType::Good},
            std::pair{std::string_view("HEALING"), DomainType::Healing},
            std::pair{std::string_view("KNOWLEDGE"), DomainType::Knowledge},
            std::pair{std::string_view("LAW"), DomainType::Law},
            std::pair{std::string_view("LIBERATION"), DomainType::Liberation},
            std::pair{std::string_view("LUCK"), DomainType::Luck},
            std::pair{std::string_view("MADNESS"), DomainType::Madness},
            std::pair{std::string_view("MAGIC"), DomainType::Magic},
            std::pair{std::string_view("NOBILITY"), DomainType::Nobility},
            std::pair{std::string_view("PLANT"), DomainType::Plant},
            std::pair{std::string_view("PROTECTION"), DomainType::Protection},
            std::pair{std::string_view("REPTILE"), DomainType::Reptile},
            std::pair{std::string_view("REPOSE"), DomainType::Repose},
            std::pair{std::string_view("RUNE"), DomainType::Rune},
            std::pair{std::string_view("STRENGTH"), DomainType::Strength},
            std::pair{std::string_view("SUN"), DomainType::Sun},
            std::pair{std::string_view("TRAVEL"), DomainType::Travel},
            std::pair{std::string_view("TRICKERY"), DomainType::Trickery},
            std::pair{std::string_view("WAR"), DomainType::War},
            std::pair{std::string_view("WATER"), DomainType::Water},
            std::pair{std::string_view("WEATHER"), DomainType::Weather},
            std::pair{std::string_view("VOID"), DomainType::Void}
        };
    };

    template<> struct EnumNames<SubdomainType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("AGATHION"), SubdomainType::Agathion},
            std::pair{std::string_view("ALIENATION"), SubdomainType::Alienation},
            std::pair{std::string_view("ANCESTORS"), SubdomainType::Ancestors},
            std::pair{std::string_view("ARCANE"), SubdomainType::Arcane},
            std::pair{std::string_view("ARCHON"), SubdomainType::Archon},
            std::pair{std::string_view("ASH"), SubdomainType::Ash},
            std::pair{std::string_view("ASSASSINATION"), SubdomainType::Assassination},
            std::pair{std::string_view("AZATA"), SubdomainType::Azata},
            std::pair{std::string_view("BLOOD"), SubdomainType::Blood},
            std::pair{std::string_view("CATASTROPHE"), SubdomainType::Catastrophe},
            std::pair{std::string_view("CAVES"), SubdomainType::Caves},
            std::pair{std::string_view("CLOUDS"), SubdomainType::Clouds},
            std::pair{std::string_view("COMMERCE"), SubdomainType::Commerce},
            std::pair{std::string_view("CONSTRUCT"), SubdomainType::Construct},
            std::pair{std::string_view("CURSE"), SubdomainType::Curse},
            std::pair{std::string_view("DARK_TAPESTRY"), SubdomainType::DarkTapestry},
            std::pair{std::string_view("DAEMON"), SubdomainType::Daemon},
            std::pair{std::string_view("DAY"), SubdomainType::Day},
            std::pair{std::string_view("DECAY"), SubdomainType::Decay},
            std::pair{std::string_view("DEFENSE"), SubdomainType::Defense},
            std::pair{std::string_view("DEMON"), SubdomainType::Demon},
            std::pair{std::string_view("DEVIL"), SubdomainType::Devil},
            std::pair{std::string_view("DIVINE"), SubdomainType::Divine},
            std::pair{std::string_view("EXPLORATION"), SubdomainType::Exploration},
            std::pair{std::string_view("FAMILY"), SubdomainType::Family},
            std::pair{std::string_view("FATE"), SubdomainType::Fate},
            std::pair{std::string_view("FEATHER"), SubdomainType::Feather},
            std::pair{std::string_view("FEROCITY"), SubdomainType::Ferocity},
            std::pair{std::string_view("FUR"), SubdomainType::Fur},
            std::pair{std::string_view("GROWTH"), SubdomainType::Growth},
            std::pair{std::string_view("HEALING"), SubdomainType::Healing},
            std::pair{std::string_view("HEROISM"), SubdomainType::Heroism},
            std::pair{std::string_view("HOME"), SubdomainType::Home},
            std::pair{std::string_view("HONOR"), SubdomainType::Honor},
            std::pair{std::string_view("ICE"), SubdomainType::Ice},
            std::pair{std::string_view("INEVITABLES"), SubdomainType::Inevitables},
            std::pair{std::string_view("INTERDICTIONS"), SubdomainType::Interdictions},
            std::pair{std::string_view("ISOLATION"), SubdomainType::Isolation},
            std::pair{std::string_view("LANGUAGE"), SubdomainType::Language},
            std::pair{std::string_view("LEADERSHIP"), SubdomainType::Leadership},
            std::pair{std::string_view("LIBERATION"), SubdomainType::Liberation},
            std::pair{std::string_view("LIGHT"), SubdomainType::Light},
            std::pair{std::string_view("LOSS"), SubdomainType::Loss},
            std::pair{std::string_view("LOYALTY"), SubdomainType::Loyalty},
            std::pair{std::string_view("LOVE"), SubdomainType::Love},
            std::pair{std::string_view("LUST"), SubdomainType::Lust},
            std::pair{std::string_view("MARTYRDOM"), SubdomainType::Martyrdom},
            std::pair{std::string_view("MEMORY"), SubdomainType::Memory},
            std::pair{std::string_view("METAL"), SubdomainType::Metal},
            std::pair{std::string_view("MOON"), SubdomainType::Moon},
            std::pair{std::string_view("NIGHT"), SubdomainType::Night},
            std::pair{std::string_view("NIGHTMARE"), SubdomainType::Nightmare},
            std::pair{std::string_view("OCEANS"), SubdomainType::Oceans},
            std::pair{std::string_view("PROTEAN"), SubdomainType::Protean},
            std::pair{std::string_view("PURITY"), SubdomainType::Purity},
            std::pair{std::string_view("RAGE"), SubdomainType::Rage},
            std::pair{std::string_view("RESOLUTION"), SubdomainType::Resolution},
            std::pair{std::string_view("RESURRECTION"), SubdomainType::Resurrection},
            std::pair{std::string_view("REVOLUTION"), SubdomainType::Revolution},
            std::pair{std::string_view("RIVERS"), SubdomainType::Rivers},
            std::pair{std::string_view("SEASONS"), SubdomainType::Seasons},
            std::pair{std::string_view("SMOKE"), SubdomainType::Smoke},
            std::pair{std::string_view("SOULS"), SubdomainType::Souls},
            std::pair{std::string_view("STARS"), SubdomainType::Stars},
            std::pair{std::string_view("STORMS"), SubdomainType::Storms},
            std::pair{std::string_view("SUBTERFUGE"), SubdomainType::Subterfuge},
            std::pair{std::string_view("TACTICS"), SubdomainType::Tactics},
            std::pair{std::string_view("THEFT"), SubdomainType::Theft},
            std::pair{std::string_view("THOUGHT"), SubdomainType::Thought},
            std::pair{std::string_view("UNDEATH"), SubdomainType::Undeath},
            std::pair{std::string_view("WHIMSY"), SubdomainType::Whimsy},
            std::pair{std::string_view("WIND"), SubdomainType::Wind}
        };
    };

    template<> struct EnumNames<BloodlineType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("ABERRANT"), BloodlineType::Aberrant},
            std::pair{std::string_view("ABYSSAL"), BloodlineType::Abyssal},
            std::pair{std::string_view("ACCURSED"), BloodlineType::Accursed},
            std::pair{std::string_view("AQUATIC"), BloodlineType::Aquatic},
            std::pair{std::string_view("ARCANE"), BloodlineType::Arcane},
            std::pair{std::string_view("CELESTIAL"), BloodlineType::Celestial},
            std::pair{std::string_view("DESTINED"), BloodlineType::Destined},
            std::pair{std::string_view("DRACONIC"), BloodlineType::Draconic},
            std::pair{std::string_view("EARTH"), BloodlineType::Earth},
            std::pair{std::string_view("ELEMENTAL"), BloodlineType::Elemental},
            std::pair{std::string_view("FEY"), BloodlineType::Fey},
            std::pair{std::string_view("IMPOSSIBLE"), BloodlineType::Impossible},
            std::pair{std::string_view("INFERNAL"), BloodlineType::Infernal},
            std::pair{std::string_view("MARID"), BloodlineType::Marid},
            std::pair{std::string_view("ONI"), BloodlineType::Oni},
            std::pair{std::string_view("PHOENIX"), BloodlineType::Phoenix},
            std::pair{std::string_view("PESTILENCE"), BloodlineType::Pestilence},
            std::pair{std::string_view("PROTEAN"), BloodlineType::Protean},
            std::pair{std::string_view("PSYCHIC"), BloodlineType::Psychic},
            std::pair{std::string_view("SERPENTINE"), BloodlineType::Serpentine},
            std::pair{std::string_view("SHADOW"), BloodlineType::Shadow},
            std::pair{std::string_view("STARSOUL"), BloodlineType::Starsoul},
            std::pair{std::string_view("STORMBORN"), BloodlineType::Stormborn},
            std::pair{std::string_view("UNDEAD"), BloodlineType::Undead},
            std::pair{std::string_view("VERMIN"), BloodlineType::Vermin}
        };
    };

    template<> struct EnumNames<CharacterClassType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("BARBARIAN"), CharacterClassType::Barbarian},
            std::pair{std::string_view("ADEPT"), CharacterClassType::Adept},
            std::pair{std::string_view("BARD"), CharacterClassType::Bard},
            std::pair{std::string_view("CLERIC"), CharacterClassType::Cleric},
            std::pair{std::string_view("DRUID"), CharacterClassType::Druid},
            std::pair{std::string_view("FIGHTER"), CharacterClassType::Fighter},
            std::pair{std::string_view("ROGUE"), CharacterClassType::Rogue},
            std::pair{std::string_view("WIZARD"), CharacterClassType::Wizard},
            std::pair{std::string_view("MONK"), CharacterClassType::Monk},
            std::pair{std::string_view("PALADIN"), CharacterClassType::Paladin},
            std::pair{std::string_view("RANGER"), CharacterClassType::Ranger},
            std::pair{std::string_view("SORCERER"), CharacterClassType::Sorcerer},
            std::pair{std::string_view("ALCHEMIST"), CharacterClassType::Alchemist},
            std::pair{std::string_view("CAVALIER"), CharacterClassType::Cavalier},
            std::pair{std::string_view("SUMMONER"), CharacterClassType::Summoner},
            std::pair{std::string_view("WITCH"), CharacterClassType::Witch},
            std::pair{std::string_view("INQUISITOR"), CharacterClassType::Inquisitor},
            std::pair{std::string_view("MAGUS"), CharacterClassType::Magus},
            std::pair{std::string_view("SHIFTER"), CharacterClassType::Shifter},
            std::pair{std::string_view("ORACLE"), CharacterClassType::Oracle},
            std::pair{std::string_view("GUNSLINGER"), CharacterClassType::Gunslinger},
            std::pair{std::string_view("VIGILANTE"), CharacterClassType::Vigilante},
            std::pair{std::string_view("ANTIPALADIN"), CharacterClassType::Antipaladin},
            std::pair{std::string_view("NINJA"), CharacterClassType::Ninja},
            std::pair{std::string_view("SAMURAI"), CharacterClassType::Samurai},
            std::pair{std::string_view("ARCANIST"), CharacterClassType::Arcanist},
            std::pair{std::string_view("BRAWLER"), CharacterClassType::Brawler},
            std::pair{std::string_view("HUNTER"), CharacterClassType::Hunter},
            std::pair{std::string_view("SWASHBUCKLER"), CharacterClassType::Swashbuckler},
            std::pair{std::string_view("INVESTIGATOR"), CharacterClassType::Investigator},
            std::pair{std::string_view("BLOODRAGER"), CharacterClassType::Bloodrager},
            std::pair{std::string_view("SLAYER"), CharacterClassType::Slayer},
            std::pair{std::string_view("WARPRIEST"), CharacterClassType::Warpriest},
            std::pair{std::string_view("SKALD"), CharacterClassType::Skald},
            std::pair{std::string_view("SHAMAN"), CharacterClassType::Shaman},
            std::pair{std::string_view("KINETICIST"), CharacterClassType::Kineticist},
            std::pair{std::string_view("MEDIUM"), CharacterClassType::Medium},
            std::pair{std::string_view("MESMERIST"), CharacterClassType::Mesmerist},
            std::pair{std::string_view("OCCULTIST"), CharacterClassType::Occultist},
            std::pair{std::string_view("PSYCHIC"), CharacterClassType::Psychic},
            std::pair{std::string_view("SPIRITUALIST"), CharacterClassType::Spiritualist},
            std::pair{std::string_view("UNCHAINED_BARBARIAN"), CharacterClassType::UnchainedBarbarian},
            std::pair{std::string_view("UNCHAINED_SUMMONER"), CharacterClassType::UnchainedSummoner},
            std::pair{std::string_view("UNCHAINED_ROGUE"), CharacterClassType::UnchainedRogue},
            std::pair{std::string_view("UNCHAINED_MONK"), CharacterClassType::UnchainedMonk}
        };
    };

    template<> struct EnumNames<SavingThrowType>
    {
        static constexpr std::array values{
            std::pair{std::string_view("FORTITUDE"), SavingThrowType::Fortitude},
            std::pair{std::string_view("REFLEX"), SavingThrowType::Reflex},
            std::pair{std::string_view("WILL"), SavingThrowType::Will}
        };
    };

    template<typename Enum> Enum enumFromJson(const Json &json)
    {
        const auto name = normalize(json.get<std::string>());
        for (const auto &[key, value] : EnumNames<Enum>::values)
            if (key == name) return value;
        throw std::invalid_argument("unknown spell enum value: " + name);
    }

    template<typename Enum> std::string enumName(Enum value)
    {
        for (const auto &[name, entry] : EnumNames<Enum>::values)
            if (entry == value) return std::string(name);
        throw std::invalid_argument("invalid spell enum value");
    }

    const Json &optionalCollection(const Json &json, const char *field, const Json &empty)
    {
        return !json.contains(field) || json.at(field).is_null() ? empty : json.at(field);
    }

    template<typename Enum> std::set<Enum> enumSet(const Json &json)
    {
        if (!json.is_array()) throw std::invalid_argument("spell enum set must be an array");
        std::set<Enum> result;
        for (const auto &entry : json)
            if (!result.insert(enumFromJson<Enum>(entry)).second)
                throw std::invalid_argument("duplicate spell enum value");
        return result;
    }

    int nonnegativeInteger(const Json &json)
    {
        if (!json.is_number_integer() || json < 0 || json > std::numeric_limits<int>::max())
            throw std::invalid_argument("spell integer must be nonnegative and fit in int");
        return json.get<int>();
    }

    template<typename Enum> std::map<Enum, int> levels(const Json &json)
    {
        if (!json.is_object()) throw std::invalid_argument("spell level map must be an object");
        std::map<Enum, int> result;
        for (auto it = json.begin(); it != json.end(); ++it)
            if (!result.emplace(enumFromJson<Enum>(it.key()), nonnegativeInteger(it.value())).second)
                throw std::invalid_argument("duplicate normalized spell list");
        return result;
    }

    std::optional<std::string> optionalString(const Json &json, const char *field)
    {
        if (!json.contains(field) || json.at(field).is_null()) return std::nullopt;
        const auto value = json.at(field).get<std::string>();
        if (value.find_first_not_of(" \t\r\n") == std::string::npos) return std::nullopt;
        return normalize(value);
    }
}

namespace golarion
{
    nlohmann::json spellDefinitionToJson(const SpellDefinition &d)
    {
        validateSpellDefinition(d);
        Json result{
            {"id", d.id}, {"name", d.name}, {"school", enumName(d.school)},
            {"castingTimeCategory", enumName(d.castingTimeCategory)}, {"castingTimeDescription", d.castingTimeDescription},
            {"rangeType", enumName(d.rangeType)}, {"rangeDescription", d.rangeDescription},
            {"durationType", enumName(d.durationType)}, {"durationDescription", d.durationDescription},
            {"dismissible", d.dismissible}, {"spellResistanceType", enumName(d.spellResistanceType)},
            {"spellResistanceDescription", d.spellResistanceDescription}, {"description", d.description}
        };
        const auto put = [&](const char *key, const auto &value) { if (value) result[key] = *value; };
        if (d.subschool) result["subschool"] = enumName(*d.subschool);
        put("materialComponentDescription", d.materialComponentDescription);
        put("materialComponentCostInGoldPieces", d.materialComponentCostInGoldPieces);
        put("focusComponentDescription", d.focusComponentDescription);
        put("target", d.target);
        put("effect", d.effect);
        put("area", d.area);
        result["descriptors"] = Json::array();
        for (const auto value : d.descriptors) result["descriptors"].push_back(enumName(value));
        result["components"] = Json::array();
        for (const auto value : d.components) result["components"].push_back(enumName(value));
        result["spellLevelByClass"] = Json::object();
        for (const auto &[type, level] : d.spellLevelByClass) result["spellLevelByClass"][enumName(type)] = level;
        result["spellLevelByBloodline"] = Json::object();
        for (const auto &[type, level] : d.spellLevelByBloodline) result["spellLevelByBloodline"][enumName(type)] = level;
        result["spellLevelByDomain"] = Json::object();
        for (const auto &[type, level] : d.spellLevelByDomain)
            std::visit([&](auto value)
            {
                const auto kind = std::holds_alternative<DomainType>(type) ? "domain" : "subdomain";
                result["spellLevelByDomain"][kind][enumName(value)] = level;
            }, type);
        result["savingThrows"] = Json::array();
        for (const auto &save : d.savingThrows)
            result["savingThrows"].push_back({{"savingThrowType", enumName(save.type)}, {"effect", enumName(save.effect)}, {"description", save.description}});
        result["effects"] = Json::array();
        result["maxTargetsExpression"] = d.maxTargetsExpression;
        for (const auto &effect : d.effects) result["effects"].push_back(effectDefinitionToJson(effect));
        return result;
    }

    SpellDefinition spellDefinitionFromJson(const nlohmann::json &json)
    {
        try
        {
            if (!json.is_object()) throw std::invalid_argument("spell definition must be an object");
            const Json emptyArray = Json::array();
            const Json emptyObject = Json::object();
            const auto text = [&](const char *field) { return normalize(json.at(field).get<std::string>()); };
            const auto subschoolName = optionalString(json, "subschool");
            std::map<DomainSpellListType, int> domainLevels;
            const auto &domains = optionalCollection(json, "spellLevelByDomain", emptyObject);
            if (!domains.is_object()) throw std::invalid_argument("spell domain lists must be an object");
            for (auto it = domains.begin(); it != domains.end(); ++it)
            {
                if (it.key() == "domain")
                    for (const auto &[type, level] : levels<DomainType>(it.value())) domainLevels.emplace(type, level);
                else if (it.key() == "subdomain")
                    for (const auto &[type, level] : levels<SubdomainType>(it.value())) domainLevels.emplace(type, level);
                else throw std::invalid_argument("unknown spell domain list kind: " + it.key());
            }
            std::vector<SpellSavingThrowDefinition> savingThrows;
            const auto &saves = optionalCollection(json, "savingThrows", emptyArray);
            if (!saves.is_array()) throw std::invalid_argument("spell savingThrows must be an array");
            for (const auto &save : saves)
                savingThrows.push_back({
                    enumFromJson<SavingThrowType>(save.at("savingThrowType")),
                    enumFromJson<SpellSavingThrowEffect>(save.at("effect")),
                    normalize(save.at("description").get<std::string>())
                });
            std::vector<EffectDefinition> effects;
            const auto &effectEntries = optionalCollection(json, "effects", emptyArray);
            if (!effectEntries.is_array()) throw std::invalid_argument("spell effects must be an array");
            for (const auto &entry : effectEntries) effects.push_back(effectDefinitionFromJson(entry));
            SpellDefinition definition{
                .id = text("id"),
                .name = text("name"),
                .school = enumFromJson<SpellSchool>(json.at("school")),
                .subschool = subschoolName ? std::optional(enumFromJson<SpellSubschool>(*subschoolName)) : std::nullopt,
                .descriptors = enumSet<SpellDescriptor>(optionalCollection(json, "descriptors", emptyArray)),
                .spellLevelByClass = levels<CharacterClassType>(optionalCollection(json, "spellLevelByClass", emptyObject)),
                .spellLevelByDomain = std::move(domainLevels),
                .spellLevelByBloodline = levels<BloodlineType>(optionalCollection(json, "spellLevelByBloodline", emptyObject)),
                .castingTimeCategory = enumFromJson<SpellCastingTimeCategory>(json.at("castingTimeCategory")),
                .castingTimeDescription = text("castingTimeDescription"),
                .components = enumSet<SpellComponentType>(optionalCollection(json, "components", emptyArray)),
                .materialComponentDescription = optionalString(json, "materialComponentDescription"),
                .materialComponentCostInGoldPieces = !json.contains("materialComponentCostInGoldPieces") || json.at("materialComponentCostInGoldPieces").is_null()
                    ? std::nullopt : std::optional(nonnegativeInteger(json.at("materialComponentCostInGoldPieces"))),
                .focusComponentDescription = optionalString(json, "focusComponentDescription"),
                .rangeType = enumFromJson<SpellRangeType>(json.at("rangeType")),
                .rangeDescription = text("rangeDescription"),
                .target = optionalString(json, "target"),
                .effect = optionalString(json, "effect"),
                .area = optionalString(json, "area"),
                .durationType = enumFromJson<SpellDurationType>(json.at("durationType")),
                .durationDescription = text("durationDescription"),
                .dismissible = !json.contains("dismissible") || json.at("dismissible").is_null() ? false : json.at("dismissible").get<bool>(),
                .savingThrows = std::move(savingThrows),
                .spellResistanceType = enumFromJson<SpellResistanceType>(json.at("spellResistanceType")),
                .spellResistanceDescription = text("spellResistanceDescription"),
                .description = text("description"),
                .effects = std::move(effects),
                .maxTargetsExpression = json.contains("maxTargetsExpression") ? text("maxTargetsExpression") : "-1"
            };
            validateSpellDefinition(definition);
            return definition;
        }
        catch (const Json::exception &exception)
        {
            throw std::invalid_argument("invalid spell definition JSON: " + std::string(exception.what()));
        }
    }
}
