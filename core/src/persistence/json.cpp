#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using Json = nlohmann::json;

    template<typename Enum, std::size_t Size>
    std::string enumName(Enum value, const std::array<std::pair<Enum, std::string_view>, Size> &names)
    {
        for (const auto &[candidate, name] : names)
        {
            if (candidate == value)
            {
                return std::string(name);
            }
        }

        throw std::invalid_argument("unknown enum value");
    }

    template<typename Enum, std::size_t Size>
    Enum enumValue(const Json &json, const std::array<std::pair<Enum, std::string_view>, Size> &names)
    {
        const std::string value = json.get<std::string>();
        for (const auto &[candidate, name] : names)
        {
            if (name == value)
            {
                return candidate;
            }
        }

        throw std::invalid_argument("unknown enum name: " + value);
    }

    constexpr std::array AbilityTypeNames{
        std::pair{golarion::AbilityType::Strength, std::string_view("strength")},
        std::pair{golarion::AbilityType::Dexterity, std::string_view("dexterity")},
        std::pair{golarion::AbilityType::Constitution, std::string_view("constitution")},
        std::pair{golarion::AbilityType::Intelligence, std::string_view("intelligence")},
        std::pair{golarion::AbilityType::Wisdom, std::string_view("wisdom")},
        std::pair{golarion::AbilityType::Charisma, std::string_view("charisma")}
    };

    constexpr std::array SkillTypeNames{
        std::pair{golarion::SkillType::Acrobatics, std::string_view("acrobatics")},
        std::pair{golarion::SkillType::HandleAnimal, std::string_view("handleAnimal")},
        std::pair{golarion::SkillType::Craft, std::string_view("craft")},
        std::pair{golarion::SkillType::EscapeArtist, std::string_view("escapeArtist")},
        std::pair{golarion::SkillType::Disguise, std::string_view("disguise")},
        std::pair{golarion::SkillType::Ride, std::string_view("ride")},
        std::pair{golarion::SkillType::KnowledgeArcana, std::string_view("knowledgeArcana")},
        std::pair{golarion::SkillType::KnowledgeDungeoneering, std::string_view("knowledgeDungeoneering")},
        std::pair{golarion::SkillType::KnowledgeEngineering, std::string_view("knowledgeEngineering")},
        std::pair{golarion::SkillType::KnowledgeGeography, std::string_view("knowledgeGeography")},
        std::pair{golarion::SkillType::KnowledgeHistory, std::string_view("knowledgeHistory")},
        std::pair{golarion::SkillType::KnowledgeLocal, std::string_view("knowledgeLocal")},
        std::pair{golarion::SkillType::KnowledgeNature, std::string_view("knowledgeNature")},
        std::pair{golarion::SkillType::KnowledgeNobility, std::string_view("knowledgeNobility")},
        std::pair{golarion::SkillType::KnowledgePlanes, std::string_view("knowledgePlanes")},
        std::pair{golarion::SkillType::KnowledgeReligion, std::string_view("knowledgeReligion")},
        std::pair{golarion::SkillType::Diplomacy, std::string_view("diplomacy")},
        std::pair{golarion::SkillType::DisableDevice, std::string_view("disableDevice")},
        std::pair{golarion::SkillType::Stealth, std::string_view("stealth")},
        std::pair{golarion::SkillType::Heal, std::string_view("heal")},
        std::pair{golarion::SkillType::Intimidate, std::string_view("intimidate")},
        std::pair{golarion::SkillType::Perform, std::string_view("perform")},
        std::pair{golarion::SkillType::SenseMotive, std::string_view("senseMotive")},
        std::pair{golarion::SkillType::Linguistics, std::string_view("linguistics")},
        std::pair{golarion::SkillType::Swim, std::string_view("swim")},
        std::pair{golarion::SkillType::Perception, std::string_view("perception")},
        std::pair{golarion::SkillType::Profession, std::string_view("profession")},
        std::pair{golarion::SkillType::Bluff, std::string_view("bluff")},
        std::pair{golarion::SkillType::SleightOfHand, std::string_view("sleightOfHand")},
        std::pair{golarion::SkillType::Spellcraft, std::string_view("spellcraft")},
        std::pair{golarion::SkillType::Climb, std::string_view("climb")},
        std::pair{golarion::SkillType::Survival, std::string_view("survival")},
        std::pair{golarion::SkillType::UseMagicDevice, std::string_view("useMagicDevice")},
        std::pair{golarion::SkillType::Appraise, std::string_view("appraise")},
        std::pair{golarion::SkillType::Fly, std::string_view("fly")}
    };

    Json abilityToJson(const golarion::AbilitySaveData &data)
    {
        return Json{
            {"type", enumName(data.type, AbilityTypeNames)},
            {"baseValue", data.baseValue}
        };
    }

    golarion::AbilitySaveData abilityFromJson(const Json &json)
    {
        return golarion::AbilitySaveData{
            .type = enumValue(json.at("type"), AbilityTypeNames),
            .baseValue = json.at("baseValue").get<int>()
        };
    }

    Json temporaryHitPointsToJson(const golarion::TemporaryHitPointsSaveData &data)
    {
        Json pools = Json::array();
        for (const golarion::TemporaryHitPointPoolSaveData &pool : data.pools)
        {
            Json remainingRounds = nullptr;
            if (pool.remainingDuration)
            {
                remainingRounds = pool.remainingDuration->roundCount();
            }
            pools.push_back(Json{
                {"id", pool.id},
                {"remaining", pool.remaining},
                {"remainingRounds", std::move(remainingRounds)}
            });
        }
        return Json{{"pools", std::move(pools)}};
    }

    golarion::TemporaryHitPointsSaveData temporaryHitPointsFromJson(const Json &json)
    {
        std::vector<golarion::TemporaryHitPointPoolSaveData> pools;
        pools.reserve(json.at("pools").size());
        for (const Json &pool : json.at("pools"))
        {
            std::optional<golarion::GameDuration> remainingDuration;
            if (!pool.at("remainingRounds").is_null())
            {
                remainingDuration = golarion::GameDuration::fromRounds(pool.at("remainingRounds").get<std::int64_t>());
            }
            pools.push_back(golarion::TemporaryHitPointPoolSaveData{
                .id = pool.at("id").get<std::string>(),
                .remaining = pool.at("remaining").get<int>(),
                .remainingDuration = remainingDuration
            });
        }
        return golarion::TemporaryHitPointsSaveData{.pools = std::move(pools)};
    }

    Json hitPointsToJson(const golarion::HitPointsSaveData &data)
    {
        return Json{
            {"baseMax", data.baseMax},
            {"damageTaken", data.damageTaken},
            {"temporary", temporaryHitPointsToJson(data.temporary)},
            {"nonLethal", data.nonLethal}
        };
    }

    golarion::HitPointsSaveData hitPointsFromJson(const Json &json)
    {
        return golarion::HitPointsSaveData{
            .baseMax = json.at("baseMax").get<int>(),
            .damageTaken = json.at("damageTaken").get<int>(),
            .temporary = temporaryHitPointsFromJson(json.at("temporary")),
            .nonLethal = json.at("nonLethal").get<int>()
        };
    }

    Json skillToJson(const golarion::SkillSaveData &data)
    {
        Json specializationId = nullptr;
        if (data.specializationId)
        {
            specializationId = *data.specializationId;
        }

        Json specialization = nullptr;
        if (data.specialization)
        {
            specialization = *data.specialization;
        }

        return Json{
            {"type", enumName(data.type, SkillTypeNames)},
            {"specializationId", std::move(specializationId)},
            {"specialization", std::move(specialization)},
            {"ranks", data.ranks},
            {"custom", data.custom}
        };
    }

    golarion::SkillSaveData skillFromJson(const Json &json)
    {
        std::optional<std::string> specializationId;
        if (!json.at("specializationId").is_null())
        {
            specializationId = json.at("specializationId").get<std::string>();
        }

        std::optional<std::string> specialization;
        if (!json.at("specialization").is_null())
        {
            specialization = json.at("specialization").get<std::string>();
        }

        return golarion::SkillSaveData{
            .type = enumValue(json.at("type"), SkillTypeNames),
            .specializationId = std::move(specializationId),
            .specialization = std::move(specialization),
            .ranks = json.at("ranks").get<int>(),
            .custom = json.at("custom").get<bool>()
        };
    }

    Json skillsToJson(const golarion::SkillsSaveData &data)
    {
        Json skills = Json::array();
        for (const golarion::SkillSaveData &skill : data.skills)
        {
            skills.push_back(skillToJson(skill));
        }
        return Json{{"skills", std::move(skills)}};
    }

    golarion::SkillsSaveData skillsFromJson(const Json &json)
    {
        std::vector<golarion::SkillSaveData> skills;
        skills.reserve(json.at("skills").size());
        for (const Json &skill : json.at("skills"))
        {
            skills.push_back(skillFromJson(skill));
        }
        return golarion::SkillsSaveData{.skills = std::move(skills)};
    }

}

namespace golarion::persistence
{
    std::string toJson(const CharacterSheetSaveData &data)
    {
        Json abilities = Json::array();
        for (const AbilitySaveData &ability : data.abilities)
        {
            abilities.push_back(abilityToJson(ability));
        }

        return Json{
            {"formatVersion", data.formatVersion},
            {"abilities", std::move(abilities)},
            {"hitPoints", hitPointsToJson(data.hitPoints)},
            {"skills", skillsToJson(data.skills)}
        }.dump(4);
    }

    CharacterSheetSaveData fromJson(std::string_view value)
    {
        const Json json = Json::parse(value.begin(), value.end());

        std::vector<AbilitySaveData> abilities;
        abilities.reserve(json.at("abilities").size());
        for (const Json &ability : json.at("abilities"))
        {
            abilities.push_back(abilityFromJson(ability));
        }

        return CharacterSheetSaveData{
            .formatVersion = json.at("formatVersion").get<int>(),
            .abilities = std::move(abilities),
            .hitPoints = hitPointsFromJson(json.at("hitPoints")),
            .skills = skillsFromJson(json.at("skills"))
        };
    }

    void saveToFile(const CharacterSheetSaveData &data, const std::filesystem::path &path)
    {
        std::ofstream file(path);
        if (!file)
        {
            throw std::runtime_error("could not open save file: " + path.string());
        }

        file << toJson(data);
        if (!file)
        {
            throw std::runtime_error("could not write save file: " + path.string());
        }
    }

    CharacterSheetSaveData loadFromFile(const std::filesystem::path &path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("could not open save file: " + path.string());
        }

        const std::string json(std::istreambuf_iterator<char>(file), {});
        if (file.bad())
        {
            throw std::runtime_error("could not read save file: " + path.string());
        }

        return fromJson(json);
    }
}
