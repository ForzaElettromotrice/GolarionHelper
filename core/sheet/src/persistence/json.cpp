#include "golarion/persistence/json.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "spellcastings_json.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
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

    constexpr std::array ModifierTypeNames{
        std::pair{golarion::ModifierType::Bonus, std::string_view("bonus")},
        std::pair{golarion::ModifierType::Penalty, std::string_view("penalty")}
    };
    constexpr std::array SkillRankBudgetNames{
        std::pair{golarion::SkillRankBudgetPolicy::Included, std::string_view("included")},
        std::pair{golarion::SkillRankBudgetPolicy::Excluded, std::string_view("excluded")}
    };
    constexpr std::array BonusTypeNames{
        std::pair{golarion::BonusType::Generic, std::string_view("generic")},
        std::pair{golarion::BonusType::Alchemical, std::string_view("alchemical")},
        std::pair{golarion::BonusType::Armor, std::string_view("armor")},
        std::pair{golarion::BonusType::NaturalArmor, std::string_view("naturalArmor")},
        std::pair{golarion::BonusType::Circumstance, std::string_view("circumstance")},
        std::pair{golarion::BonusType::Insight, std::string_view("insight")},
        std::pair{golarion::BonusType::Competence, std::string_view("competence")},
        std::pair{golarion::BonusType::Deflection, std::string_view("deflection")},
        std::pair{golarion::BonusType::Luck, std::string_view("luck")},
        std::pair{golarion::BonusType::Inherent, std::string_view("inherent")},
        std::pair{golarion::BonusType::Morale, std::string_view("morale")},
        std::pair{golarion::BonusType::Enhancement, std::string_view("enhancement")},
        std::pair{golarion::BonusType::Profane, std::string_view("profane")},
        std::pair{golarion::BonusType::Racial, std::string_view("racial")},
        std::pair{golarion::BonusType::Resistance, std::string_view("resistance")},
        std::pair{golarion::BonusType::Sacred, std::string_view("sacred")},
        std::pair{golarion::BonusType::Dodge, std::string_view("dodge")},
        std::pair{golarion::BonusType::Shield, std::string_view("shield")},
        std::pair{golarion::BonusType::Size, std::string_view("size")}
    };

    Json enhancementsToJson(const golarion::EnhancementsData &data)
    {
        Json groups = Json::array();
        for (const auto &group : data.groups)
        {
            Json entries = Json::array();
            for (const auto &entry : group.enhancements)
            {
                entries.push_back(std::visit([&](const auto &definition)
                {
                    Json json{{"id", definition.id}, {"resource", definition.resource}, {"expression", definition.expression}, {"active", entry.active}};
                    if constexpr (std::is_same_v<std::decay_t<decltype(definition)>, golarion::ModifierEffectDefinition>)
                    {
                        json["type"] = "modifier";
                        json["description"] = definition.description;
                        json["modifierType"] = enumName(definition.type, ModifierTypeNames);
                        json["skillRankBudget"] = enumName(definition.skillRankBudget, SkillRankBudgetNames);
                        if (definition.bonusType) json["bonusType"] = enumName(*definition.bonusType, BonusTypeNames);
                        if (definition.condition) json["condition"] = *definition.condition;
                    }
                    else json["type"] = "contribution";
                    return json;
                }, entry.definition));
            }
            groups.push_back(Json{{"id", group.id}, {"name", group.name}, {"enhancements", std::move(entries)}});
        }
        return groups;
    }

    golarion::EnhancementsData enhancementsFromJson(const Json &json)
    {
        using namespace golarion;
        if (!json.is_array()) throw std::invalid_argument("enhancement groups must be an array");
        EnhancementsData data;
        for (const auto &group : json)
        {
            EnhancementGroupData result{group.at("id").get<std::string>(), group.at("name").get<std::string>(), {}};
            if (!group.at("enhancements").is_array()) throw std::invalid_argument("enhancements must be an array");
            for (const auto &entry : group.at("enhancements"))
            {
                const auto id = entry.at("id").get<std::string>();
                const auto resource = entry.at("resource").get<std::string>();
                const auto expression = entry.at("expression").get<std::string>();
                const bool active = entry.at("active").get<bool>();
                const auto type = entry.at("type").get<std::string>();
                if (type == "contribution") result.enhancements.push_back({ContributionEffectDefinition{id, resource, expression}, active});
                else if (type == "modifier")
                {
                    ModifierEffectDefinition definition{id, resource, entry.at("description").get<std::string>(), enumValue(entry.at("modifierType"), ModifierTypeNames), {}, expression};
                    if (entry.contains("bonusType")) definition.bonusType = enumValue(entry.at("bonusType"), BonusTypeNames);
                    if ((definition.type == ModifierType::Bonus) != definition.bonusType.has_value()) throw std::invalid_argument("invalid enhancement bonus type");
                    if (entry.contains("condition")) definition.condition = entry.at("condition").get<std::string>();
                    definition.skillRankBudget = enumValue(entry.at("skillRankBudget"), SkillRankBudgetNames);
                    result.enhancements.push_back({std::move(definition), active});
                }
                else throw std::invalid_argument("unknown enhancement type: " + type);
            }
            data.groups.push_back(std::move(result));
        }
        return data;
    }

    constexpr std::array AbilityTypeNames{
        std::pair{golarion::AbilityType::Strength, std::string_view("strength")},
        std::pair{golarion::AbilityType::Dexterity, std::string_view("dexterity")},
        std::pair{golarion::AbilityType::Constitution, std::string_view("constitution")},
        std::pair{golarion::AbilityType::Intelligence, std::string_view("intelligence")},
        std::pair{golarion::AbilityType::Wisdom, std::string_view("wisdom")},
        std::pair{golarion::AbilityType::Charisma, std::string_view("charisma")}
    };

    constexpr std::array AlignmentNames{
        std::pair{golarion::Alignment::LawfulGood, std::string_view("lawfulGood")},
        std::pair{golarion::Alignment::NeutralGood, std::string_view("neutralGood")},
        std::pair{golarion::Alignment::ChaoticGood, std::string_view("chaoticGood")},
        std::pair{golarion::Alignment::LawfulNeutral, std::string_view("lawfulNeutral")},
        std::pair{golarion::Alignment::Neutral, std::string_view("neutral")},
        std::pair{golarion::Alignment::ChaoticNeutral, std::string_view("chaoticNeutral")},
        std::pair{golarion::Alignment::LawfulEvil, std::string_view("lawfulEvil")},
        std::pair{golarion::Alignment::NeutralEvil, std::string_view("neutralEvil")},
        std::pair{golarion::Alignment::ChaoticEvil, std::string_view("chaoticEvil")}
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

    template<typename Value>
    Json optionalToJson(const std::optional<Value> &value)
    {
        return value.has_value() ? Json(*value) : Json(nullptr);
    }

    template<typename Value>
    std::optional<Value> optionalFromJson(const Json &json, std::string_view field)
    {
        const std::string key(field);
        if (!json.contains(key) || json.at(key).is_null())
        {
            return std::nullopt;
        }
        return json.at(key).get<Value>();
    }

    Json characterIdentityToJson(const golarion::CharacterIdentitySaveData &data)
    {
        Json alignment = nullptr;
        if (data.alignment.has_value())
        {
            alignment = enumName(*data.alignment, AlignmentNames);
        }
        return Json{
            {"name", data.name},
            {"playerName", data.playerName},
            {"alignment", std::move(alignment)},
            {"deity", optionalToJson(data.deity)},
            {"homeland", optionalToJson(data.homeland)},
            {"gender", optionalToJson(data.gender)},
            {"age", optionalToJson(data.age)},
            {"heightCentimeters", optionalToJson(data.heightCentimeters)},
            {"weightGrams", optionalToJson(data.weightGrams)},
            {"hair", optionalToJson(data.hair)},
            {"eyes", optionalToJson(data.eyes)},
            {"appearance", optionalToJson(data.appearance)}
        };
    }

    golarion::CharacterIdentitySaveData characterIdentityFromJson(const Json &json)
    {
        std::optional<golarion::Alignment> alignment;
        if (json.contains("alignment") && !json.at("alignment").is_null())
        {
            alignment = enumValue(json.at("alignment"), AlignmentNames);
        }
        return golarion::CharacterIdentitySaveData{
            .name = json.value("name", ""),
            .playerName = json.value("playerName", ""),
            .alignment = alignment,
            .deity = optionalFromJson<std::string>(json, "deity"),
            .homeland = optionalFromJson<std::string>(json, "homeland"),
            .gender = optionalFromJson<std::string>(json, "gender"),
            .age = optionalFromJson<int>(json, "age"),
            .heightCentimeters = optionalFromJson<int>(json, "heightCentimeters"),
            .weightGrams = optionalFromJson<std::int64_t>(json, "weightGrams"),
            .hair = optionalFromJson<std::string>(json, "hair"),
            .eyes = optionalFromJson<std::string>(json, "eyes"),
            .appearance = optionalFromJson<std::string>(json, "appearance")
        };
    }

    constexpr std::array LanguageOriginNames{
        std::pair{golarion::LanguageOrigin::Intelligence, std::string_view("intelligence")},
        std::pair{golarion::LanguageOrigin::Linguistics, std::string_view("linguistics")}
    };

    Json languagesToJson(const golarion::LanguagesData &data, int formatVersion)
    {
        Json learned = Json::array();
        for (const auto &entry : data.learned)
            learned.push_back({{"languageId", entry.languageId}, {"origin", enumName(entry.origin, LanguageOriginNames)}});
        Json result{{"learned", std::move(learned)}};
        if (formatVersion < 35) result["intelligenceBonus"] = 0;
        return result;
    }

    golarion::LanguagesData languagesFromJson(const Json &json)
    {
        if (!json.at("learned").is_array()) throw std::invalid_argument("learned languages must be an array");
        // Legacy entitlement is validated but no longer overrides permanent Intelligence.
        if (json.contains("intelligenceBonus"))
        {
            const auto &bonus = json.at("intelligenceBonus");
            if (!bonus.is_number_integer() || bonus < 0 || bonus > std::numeric_limits<int>::max())
                throw std::invalid_argument("intelligence language bonus must be a nonnegative integer");
        }
        golarion::LanguagesData data;
        for (const auto &entry : json.at("learned"))
            data.learned.push_back({entry.at("languageId").get<std::string>(), enumValue(entry.at("origin"), LanguageOriginNames)});
        return data;
    }

    Json grantedSpellsToJson(const golarion::GrantedSpellsData &data)
    {
        Json spells = Json::array();
        for (const auto &entry : data.spells) spells.push_back({{"id", entry.id}, {"remaining", entry.remaining}});
        return spells;
    }

    golarion::GrantedSpellsData grantedSpellsFromJson(const Json &json)
    {
        if (!json.is_array()) throw std::invalid_argument("granted spell state must be an array");
        golarion::GrantedSpellsData data;
        for (const auto &entry : json)
        {
            const auto &count = entry.at("remaining");
            if (!count.is_number_integer() || count < 0 || count > std::numeric_limits<int>::max()) throw std::invalid_argument("invalid granted spell remaining uses");
            data.spells.push_back({entry.at("id").get<std::string>(), count.get<int>()});
        }
        return data;
    }

    Json favoredClassesToJson(const golarion::FavoredClassesData &data)
    {
        Json selected = Json::array(), bonuses = Json::array();
        for (auto type : data.selectedClasses) selected.push_back(golarion::characterClassId(type));
        for (const auto &bonus : data.bonuses)
            bonuses.push_back({{"type", golarion::characterClassId(bonus.type)}, {"classLevel", bonus.classLevel}, {"optionId", bonus.optionId}});
        return {{"selectedClasses", std::move(selected)}, {"bonuses", std::move(bonuses)}};
    }

    golarion::FavoredClassesData favoredClassesFromJson(const Json &json)
    {
        golarion::FavoredClassesData result;
        if (!json.at("selectedClasses").is_array() || !json.at("bonuses").is_array()) throw std::invalid_argument("favored class selections must be arrays");
        for (const auto &value : json.at("selectedClasses"))
            if (!result.selectedClasses.insert(golarion::characterClassFromId(value.get<std::string>())).second)
                throw std::invalid_argument("duplicate favored class");
        std::set<std::pair<golarion::CharacterClassType, int>> keys;
        for (const auto &value : json.at("bonuses"))
        {
            const auto type = golarion::characterClassFromId(value.at("type").get<std::string>());
            const auto &level = value.at("classLevel");
            const auto id = value.at("optionId").get<std::string>();
            if (!level.is_number_integer() || level < 1 || level > std::numeric_limits<int>::max() || id.empty() || !keys.emplace(type, level.get<int>()).second)
                throw std::invalid_argument("invalid or duplicate favored class bonus");
            result.bonuses.push_back({type, level.get<int>(), id});
        }
        return result;
    }

    Json classesToJson(const golarion::ClassesData &data)
    {
        Json result = Json::array();
        for (const auto &entry : data.classes)
        {
            Json choices = Json::array();
            for (const auto &choice : entry.choices)
                choices.push_back({{"acquisitionId", choice.acquisitionId}, {"choiceId", choice.choiceId}, {"optionIds", choice.optionIds}});
            Json dice = Json::array();
            for (const auto &[level, value] : entry.hitDieResults) dice.push_back({{"level", level}, {"result", value}});
            result.push_back({{"type", golarion::characterClassId(entry.type)}, {"level", entry.level}, {"choices", std::move(choices)}, {"archetypeIds", entry.archetypeIds}, {"hitDieResults", std::move(dice)}});
        }
        return result;
    }

    golarion::ClassesData classesFromJson(const Json &json, bool hasArchetypes, bool hasHitDice)
    {
        if (!json.is_array()) throw std::invalid_argument("classes must be an array");
        golarion::ClassesData result;
        std::set<golarion::CharacterClassType> types;
        long long total = 0;
        for (const auto &entry : json)
        {
            const auto type = golarion::characterClassFromId(entry.at("type").get<std::string>());
            const auto &level = entry.at("level");
            if (!level.is_number_integer() || level < 1 || level > std::numeric_limits<int>::max() || !types.insert(type).second)
                throw std::invalid_argument("invalid saved class level or duplicate class");
            golarion::CharacterClassData value{type, level.get<int>(), {}};
            if (hasHitDice)
            {
                if (!entry.at("hitDieResults").is_array()) throw std::invalid_argument("hit die results must be an array");
                for (const auto &die : entry.at("hitDieResults"))
                {
                    const auto &dieLevel = die.at("level");
                    const auto &result = die.at("result");
                    if (!dieLevel.is_number_integer() || dieLevel < 1 || dieLevel > value.level ||
                        !result.is_number_integer() || result < 1 || result > 12 ||
                        !value.hitDieResults.emplace(dieLevel.get<int>(), result.get<int>()).second)
                        throw std::invalid_argument("invalid or duplicate saved hit die result");
                }
            }
            if (hasArchetypes)
            {
                if (!entry.at("archetypeIds").is_array()) throw std::invalid_argument("archetype IDs must be an array");
                std::set<std::string> ids;
                for (const auto &item : entry.at("archetypeIds"))
                {
                    auto id = item.get<std::string>();
                    if (id.empty() || !ids.insert(id).second) throw std::invalid_argument("invalid or duplicate saved archetype");
                    value.archetypeIds.push_back(std::move(id));
                }
            }
            total += value.level;
            if (total > std::numeric_limits<int>::max()) throw std::invalid_argument("total class level exceeds integer range");
            if (!entry.at("choices").is_array()) throw std::invalid_argument("class choices must be an array");
            std::set<std::pair<std::string, std::string>> keys;
            for (const auto &choice : entry.at("choices"))
            {
                golarion::ClassChoiceData saved{choice.at("acquisitionId").get<std::string>(), choice.at("choiceId").get<std::string>(), {}};
                if (saved.acquisitionId.empty() || saved.choiceId.empty() || !keys.emplace(saved.acquisitionId, saved.choiceId).second)
                    throw std::invalid_argument("invalid or duplicate saved class choice");
                if (!choice.at("optionIds").is_array()) throw std::invalid_argument("class option IDs must be an array");
                std::set<std::string> options;
                for (const auto &option : choice.at("optionIds"))
                {
                    auto id = option.get<std::string>();
                    if (id.empty() || !options.insert(id).second) throw std::invalid_argument("invalid or duplicate saved class option");
                    saved.optionIds.push_back(std::move(id));
                }
                value.choices.push_back(std::move(saved));
            }
            result.classes.push_back(std::move(value));
        }
        return result;
    }

    Json featsToJson(const golarion::FeatsData &data)
    {
        Json selections = Json::array();
        for (const auto &selection : data.selections)
            selections.push_back({{"slotId", selection.slotId}, {"featId", selection.featId}, {"choices", selection.choices}});
        return Json{{"selections", std::move(selections)}, {"extraSlotIds", data.extraSlotIds}};
    }

    golarion::FeatsData featsFromJson(const Json &json, bool extraSlots)
    {
        const auto &selections = json.at("selections");
        if (!selections.is_array()) throw std::invalid_argument("feat selections must be an array");
        golarion::FeatsData data;
        if (extraSlots)
        {
            const auto &ids = json.at("extraSlotIds");
            if (!ids.is_array()) throw std::invalid_argument("extra feat slot IDs must be an array");
            data.extraSlotIds = ids.get<std::vector<std::string>>();
        }
        for (const auto &selection : selections)
            data.selections.push_back({selection.at("slotId").get<std::string>(), selection.at("featId").get<std::string>(),
                selection.at("choices").get<std::map<std::string, std::vector<std::string>>>()});
        return data;
    }

    Json raceToJson(const golarion::RaceSaveData &data)
    {
        Json choices = Json::array();
        for (const golarion::RacialChoiceSelectionSaveData &choice : data.choices)
        {
            choices.push_back(Json{
                {"elementId", choice.elementId},
                {"choiceId", choice.choiceId},
                {"optionIds", choice.optionIds}
            });
        }
        return Json{
            {"raceDefinitionId", optionalToJson(data.raceDefinitionId)},
            {"alternateFeatureIds", data.alternateFeatureIds},
            {"choices", std::move(choices)}
        };
    }

    golarion::RaceSaveData raceFromJson(const Json &json)
    {
        std::vector<golarion::RacialChoiceSelectionSaveData> choices;
        choices.reserve(json.at("choices").size());
        for (const Json &choice : json.at("choices"))
        {
            choices.push_back(golarion::RacialChoiceSelectionSaveData{
                .elementId = choice.at("elementId").get<std::string>(),
                .choiceId = choice.at("choiceId").get<std::string>(),
                .optionIds = choice.at("optionIds").get<std::vector<std::string>>()
            });
        }
        return golarion::RaceSaveData{
            .raceDefinitionId = optionalFromJson<std::string>(json, "raceDefinitionId"),
            .alternateFeatureIds = json.at("alternateFeatureIds").get<std::vector<std::string>>(),
            .choices = std::move(choices)
        };
    }

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
            Json durationRounds = nullptr;
            if (pool.duration.has_value())
            {
                durationRounds = pool.duration->roundCount();
            }
            pools.push_back(Json{
                {"id", pool.id},
                {"remaining", pool.remaining},
                {"durationRounds", std::move(durationRounds)}
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
            std::optional<golarion::GameDuration> duration;
            if (pool.contains("durationRounds") && !pool.at("durationRounds").is_null())
            {
                duration = golarion::GameDuration::fromRounds(pool.at("durationRounds").get<std::int64_t>());
            }
            else if (pool.contains("remainingRounds") && !pool.at("remainingRounds").is_null())
            {
                duration = golarion::GameDuration::fromRounds(pool.at("remainingRounds").get<std::int64_t>());
            }
            pools.push_back(golarion::TemporaryHitPointPoolSaveData{
                .id = pool.at("id").get<std::string>(),
                .remaining = pool.at("remaining").get<int>(),
                .duration = duration
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
            {"nonLethal", data.nonLethal},
            {"dead", data.dead}
        };
    }

    golarion::HitPointsSaveData hitPointsFromJson(const Json &json)
    {
        return golarion::HitPointsSaveData{
            .baseMax = json.at("baseMax").get<int>(),
            .damageTaken = json.at("damageTaken").get<int>(),
            .temporary = temporaryHitPointsFromJson(json.at("temporary")),
            .nonLethal = json.at("nonLethal").get<int>(),
            .dead = json.value("dead", false)
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

    Json attacksToJson(const golarion::AttacksData &data)
    {
        Json attacks = Json::array();
        for (const golarion::AttackData &attack : data.attacks)
        {
            Json assignments = Json::array();
            for (const golarion::AttackAssignmentData &assignment : attack.assignments)
            {
                assignments.push_back(Json{
                    {"slotId", assignment.slotId},
                    {"strikeGrantId", assignment.strikeGrantId}
                });
            }
            attacks.push_back(Json{
                {"id", attack.id},
                {"name", attack.name},
                {"routineId", attack.routineId},
                {"assignments", std::move(assignments)}
            });
        }
        return Json{{"attacks", std::move(attacks)}};
    }

    golarion::AttacksData attacksFromJson(const Json &json)
    {
        std::vector<golarion::AttackData> attacks;
        attacks.reserve(json.at("attacks").size());
        for (const Json &attack : json.at("attacks"))
        {
            std::vector<golarion::AttackAssignmentData> assignments;
            assignments.reserve(attack.at("assignments").size());
            for (const Json &assignment : attack.at("assignments"))
            {
                assignments.push_back(golarion::AttackAssignmentData{
                    .slotId = assignment.at("slotId").get<std::string>(),
                    .strikeGrantId = assignment.at("strikeGrantId").get<std::string>()
                });
            }
            attacks.push_back(golarion::AttackData{
                .id = attack.at("id").get<std::string>(),
                .name = attack.at("name").get<std::string>(),
                .routineId = attack.at("routineId").get<std::string>(),
                .assignments = std::move(assignments)
            });
        }
        return golarion::AttacksData{.attacks = std::move(attacks)};
    }

    Json itemSelectorToJson(const golarion::ItemSelectorSaveData &data)
    {
        return Json{
            {"definitionIds", data.definitionIds},
            {"tags", data.tags}
        };
    }

    golarion::ItemSelectorSaveData itemSelectorFromSaveJson(const Json &json)
    {
        return golarion::ItemSelectorSaveData{
            .definitionIds = json.at("definitionIds").get<std::vector<std::string>>(),
            .tags = json.at("tags").get<std::vector<std::string>>()
        };
    }

    Json inventoryToJson(const golarion::InventorySaveData &data)
    {
        Json containers = Json::array();
        for (const golarion::InventoryContainerSaveData &container : data.containers)
        {
            Json maximumContentsWeightGrams = nullptr;
            if (container.maximumContentsWeightGrams.has_value())
            {
                maximumContentsWeightGrams = *container.maximumContentsWeightGrams;
            }
            Json maximumContentsVolumeMilliliters = nullptr;
            if (container.maximumContentsVolumeMilliliters.has_value())
            {
                maximumContentsVolumeMilliliters = *container.maximumContentsVolumeMilliliters;
            }
            Json acceptedItems = nullptr;
            if (container.acceptedItems.has_value())
            {
                acceptedItems = itemSelectorToJson(*container.acceptedItems);
            }
            Json quantityLimits = Json::array();
            for (const golarion::ItemQuantityLimitSaveData &limit : container.quantityLimits)
            {
                Json selector = nullptr;
                if (limit.selector.has_value())
                {
                    selector = itemSelectorToJson(*limit.selector);
                }
                quantityLimits.push_back(Json{
                    {"maximumQuantity", limit.maximumQuantity},
                    {"selector", std::move(selector)}
                });
            }
            containers.push_back(Json{
                {"id", container.id},
                {"name", container.name},
                {"maximumContentsWeightGrams", std::move(maximumContentsWeightGrams)},
                {"maximumContentsVolumeMilliliters", std::move(maximumContentsVolumeMilliliters)},
                {"acceptedItems", std::move(acceptedItems)},
                {"quantityLimits", std::move(quantityLimits)},
                {"ignoresContentsWeight", container.ignoresContentsWeight},
                {"ignoresContentsVolume", container.ignoresContentsVolume},
                {"allowsPossessionEffects", container.allowsPossessionEffects},
                {"contributesToCarriedWeight", container.contributesToCarriedWeight}
            });
        }

        Json items = Json::array();
        for (const golarion::InventoryItemSaveData &item : data.items)
        {
            Json choices = Json::array();
            for (const golarion::ItemChoiceSelectionSaveData &choice : item.choices)
            {
                choices.push_back(Json{
                    {"choiceId", choice.choiceId},
                    {"optionIds", choice.optionIds}
                });
            }
            items.push_back(Json{
                {"id", item.id},
                {"itemDefinitionId", item.itemDefinitionId},
                {"quantity", item.quantity},
                {"choices", std::move(choices)},
                {"containerId", item.containerId},
                {"equipped", item.equipped},
                {"remainingUses", item.remainingUses ? Json(*item.remainingUses) : Json(nullptr)}
            });
        }
        return Json{
            {"containers", std::move(containers)},
            {"items", std::move(items)}
        };
    }

    golarion::InventorySaveData inventoryFromJson(const Json &json, bool requireUsageState)
    {
        std::vector<golarion::InventoryContainerSaveData> containers;
        containers.reserve(json.at("containers").size());
        for (const Json &container : json.at("containers"))
        {
            std::optional<std::int64_t> maximumContentsWeightGrams;
            if (!container.at("maximumContentsWeightGrams").is_null())
            {
                maximumContentsWeightGrams = container.at("maximumContentsWeightGrams").get<std::int64_t>();
            }
            std::optional<std::int64_t> maximumContentsVolumeMilliliters;
            if (!container.at("maximumContentsVolumeMilliliters").is_null())
            {
                maximumContentsVolumeMilliliters = container.at("maximumContentsVolumeMilliliters").get<std::int64_t>();
            }
            std::optional<golarion::ItemSelectorSaveData> acceptedItems;
            if (!container.at("acceptedItems").is_null())
            {
                acceptedItems = itemSelectorFromSaveJson(container.at("acceptedItems"));
            }
            std::vector<golarion::ItemQuantityLimitSaveData> quantityLimits;
            quantityLimits.reserve(container.at("quantityLimits").size());
            for (const Json &limit : container.at("quantityLimits"))
            {
                std::optional<golarion::ItemSelectorSaveData> selector;
                if (!limit.at("selector").is_null())
                {
                    selector = itemSelectorFromSaveJson(limit.at("selector"));
                }
                quantityLimits.push_back(golarion::ItemQuantityLimitSaveData{
                    .maximumQuantity = limit.at("maximumQuantity").get<std::int64_t>(),
                    .selector = std::move(selector)
                });
            }
            containers.push_back(golarion::InventoryContainerSaveData{
                .id = container.at("id").get<std::string>(),
                .name = container.at("name").get<std::string>(),
                .maximumContentsWeightGrams = maximumContentsWeightGrams,
                .maximumContentsVolumeMilliliters = maximumContentsVolumeMilliliters,
                .acceptedItems = std::move(acceptedItems),
                .quantityLimits = std::move(quantityLimits),
                .ignoresContentsWeight = container.at("ignoresContentsWeight").get<bool>(),
                .ignoresContentsVolume = container.at("ignoresContentsVolume").get<bool>(),
                .allowsPossessionEffects = container.at("allowsPossessionEffects").get<bool>(),
                .contributesToCarriedWeight = container.value("contributesToCarriedWeight", false)
            });
        }

        std::vector<golarion::InventoryItemSaveData> items;
        items.reserve(json.at("items").size());
        for (const Json &item : json.at("items"))
        {
            std::vector<golarion::ItemChoiceSelectionSaveData> choices;
            for (const Json &choice : item.value("choices", Json::array()))
            {
                choices.push_back(golarion::ItemChoiceSelectionSaveData{
                    .choiceId = choice.at("choiceId").get<std::string>(),
                    .optionIds = choice.at("optionIds").get<std::vector<std::string>>()
                });
            }
            std::optional<std::map<std::string, int>> remainingUses;
            if (requireUsageState || (item.contains("remainingUses") && !item.at("remainingUses").is_null()))
            {
                const auto &states = item.at("remainingUses");
                if (!states.is_object()) throw std::invalid_argument("item remaining uses must be an object");
                remainingUses.emplace();
                for (const auto &[id, count] : states.items())
                {
                    if (!count.is_number_integer() || count < 0 || count > std::numeric_limits<int>::max())
                        throw std::invalid_argument("item remaining uses must be nonnegative integers");
                    remainingUses->emplace(id, count.get<int>());
                }
            }
            items.push_back(golarion::InventoryItemSaveData{
                .id = item.at("id").get<std::string>(),
                .itemDefinitionId = item.at("itemDefinitionId").get<std::string>(),
                .quantity = item.at("quantity").get<int>(),
                .choices = std::move(choices),
                .containerId = item.at("containerId").get<std::string>(),
                .equipped = item.at("equipped").get<bool>(),
                .remainingUses = std::move(remainingUses)
            });
        }
        return golarion::InventorySaveData{
            .containers = std::move(containers),
            .items = std::move(items)
        };
    }

    Json conditionsToJson(const golarion::ConditionManagerSaveData &data)
    {
        Json manualEntries = Json::array();
        for (const golarion::ConditionEntrySaveData &entry : data.manualEntries)
        {
            Json stackingGroup = nullptr;
            if (entry.stackingGroup.has_value())
            {
                stackingGroup = *entry.stackingGroup;
            }
            Json parameter = nullptr;
            if (entry.parameter.has_value())
            {
                parameter = *entry.parameter;
            }
            manualEntries.push_back(Json{
                {"id", entry.id},
                {"conditionId", entry.conditionId},
                {"source", entry.source},
                {"severity", entry.severity},
                {"contributesToEscalation", entry.contributesToEscalation},
                {"stackingGroup", std::move(stackingGroup)},
                {"parameter", std::move(parameter)}
            });
        }
        return Json{{"manualEntries", std::move(manualEntries)}};
    }

    golarion::ConditionManagerSaveData conditionsFromJson(const Json &json)
    {
        std::vector<golarion::ConditionEntrySaveData> manualEntries;
        manualEntries.reserve(json.at("manualEntries").size());
        for (const Json &entry : json.at("manualEntries"))
        {
            std::optional<std::string> stackingGroup;
            if (!entry.at("stackingGroup").is_null())
            {
                stackingGroup = entry.at("stackingGroup").get<std::string>();
            }
            std::optional<std::string> parameter;
            if (entry.contains("parameter") && !entry.at("parameter").is_null())
            {
                parameter = entry.at("parameter").get<std::string>();
            }
            manualEntries.push_back(golarion::ConditionEntrySaveData{
                .id = entry.at("id").get<std::string>(),
                .conditionId = entry.at("conditionId").get<std::string>(),
                .source = entry.at("source").get<std::string>(),
                .severity = entry.at("severity").get<int>(),
                .contributesToEscalation = entry.at("contributesToEscalation").get<bool>(),
                .stackingGroup = std::move(stackingGroup),
                .parameter = std::move(parameter)
            });
        }
        return golarion::ConditionManagerSaveData{.manualEntries = std::move(manualEntries)};
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

        Json customFeats = Json::array();
        for (const auto &definition : data.customFeatDefinitions) customFeats.push_back(featDefinitionToJson(definition));
        Json customSpells = Json::array();
        for (const auto &definition : data.customSpellDefinitions) customSpells.push_back(spellDefinitionToJson(definition));
        Json customItems = Json::array();
        for (const auto &definition : data.customItemDefinitions) customItems.push_back(itemDefinitionToJson(definition));
        return Json{
            {"customItemDefinitions", std::move(customItems)},
            {"customSpellDefinitions", std::move(customSpells)},
            {"customFeatDefinitions", std::move(customFeats)},
            {"formatVersion", data.formatVersion},
            {"notes", data.notes},
            {"identity", characterIdentityToJson(data.identity)},
            {"race", raceToJson(data.race)},
            {"abilities", std::move(abilities)},
            {"hitPoints", hitPointsToJson(data.hitPoints)},
            {"skills", skillsToJson(data.skills)},
            {"attacks", attacksToJson(data.attacks)},
            {"conditions", conditionsToJson(data.conditions)},
            {"inventory", inventoryToJson(data.inventory)},
            {"feats", featsToJson(data.feats)},
            {"languages", languagesToJson(data.languages, data.formatVersion)},
            {"spellcastings", spellcastingsToJson(data.spellcastings, data.formatVersion >= 34, data.formatVersion >= 40)},
            {"metamagicUses", data.formatVersion >= 34 ? metamagicUsesToJson(data.spellcastings.metamagicUses) : Json(nullptr)},
            {"grantedSpells", grantedSpellsToJson(data.grantedSpells)},
            {"classes", classesToJson(data.classes)},
            {"favoredClasses", favoredClassesToJson(data.classes.favoredClasses)},
            {"legacyFirstLevelFeatSlot", data.legacyFirstLevelFeatSlot},
            {"enhancements", enhancementsToJson(data.enhancements)}
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

        const int formatVersion = json.at("formatVersion").get<int>();
        std::vector<FeatDefinition> customFeats;
        if (formatVersion >= 39)
        {
            const auto &definitions = json.at("customFeatDefinitions");
            if (!definitions.is_array()) throw std::invalid_argument("custom feat definitions must be an array");
            for (const auto &definition : definitions) customFeats.push_back(featDefinitionFromJson(definition));
        }
        std::vector<SpellDefinition> customSpells;
        if (formatVersion >= 38)
        {
            const auto &definitions = json.at("customSpellDefinitions");
            if (!definitions.is_array()) throw std::invalid_argument("custom spell definitions must be an array");
            for (const auto &definition : definitions) customSpells.push_back(spellDefinitionFromJson(definition));
        }
        std::vector<ItemDefinition> customItems;
        if (formatVersion >= 37)
        {
            const auto &definitions = json.at("customItemDefinitions");
            if (!definitions.is_array()) throw std::invalid_argument("custom item definitions must be an array");
            for (const auto &definition : definitions) customItems.push_back(itemDefinitionFromJson(definition));
        }
        auto classes = formatVersion >= 30 ? classesFromJson(json.at("classes"), formatVersion >= 31, formatVersion >= 32) : ClassesData{};
        if (formatVersion >= 33) classes.favoredClasses = favoredClassesFromJson(json.at("favoredClasses"));
        auto spellcastings = formatVersion >= 27 ? spellcastingsFromJson(json.at("spellcastings"), formatVersion >= 34, formatVersion >= 40) : SpellcastingsData{};
        if (formatVersion >= 34) spellcastings.metamagicUses = metamagicUsesFromJson(json.at("metamagicUses"));
        return CharacterSheetSaveData{
            .formatVersion = formatVersion,
            .identity = formatVersion >= 22 ? characterIdentityFromJson(json.at("identity")) : CharacterIdentitySaveData{},
            .race = formatVersion >= 24 ? raceFromJson(json.at("race")) : RaceSaveData{},
            .abilities = std::move(abilities),
            .hitPoints = hitPointsFromJson(json.at("hitPoints")),
            .skills = skillsFromJson(json.at("skills")),
            .attacks = json.contains("attacks") ? attacksFromJson(json.at("attacks")) : AttacksData{},
            .conditions = formatVersion >= 17 ? conditionsFromJson(json.at("conditions")) : ConditionManagerSaveData{},
            .inventory = formatVersion >= 19 ? inventoryFromJson(json.at("inventory"), formatVersion >= 28) : InventorySaveData{},
            .feats = formatVersion >= 25 ? featsFromJson(json.at("feats"), formatVersion >= 39) : FeatsData{},
            .languages = formatVersion >= 26 ? languagesFromJson(json.at("languages")) : LanguagesData{},
            .spellcastings = std::move(spellcastings),
            .grantedSpells = formatVersion >= 29 ? grantedSpellsFromJson(json.at("grantedSpells")) : GrantedSpellsData{},
            .classes = std::move(classes),
            .legacyFirstLevelFeatSlot = formatVersion >= 30 ? json.at("legacyFirstLevelFeatSlot").get<bool>() : false,
            .enhancements = formatVersion >= 36 ? enhancementsFromJson(json.at("enhancements")) : EnhancementsData{},
            .customItemDefinitions = std::move(customItems),
            .customSpellDefinitions = std::move(customSpells),
            .customFeatDefinitions = std::move(customFeats),
            .notes = formatVersion >= 41 ? json.at("notes").get<std::string>() : std::string{}
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
