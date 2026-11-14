#include "golarion/character/class_definition_json.hpp"
#include "golarion/character/archetype_definition_json.hpp"

#include "golarion/effect/effect_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;
    using namespace golarion;

    constexpr std::array SkillTypes{
        SkillType::Acrobatics, SkillType::HandleAnimal, SkillType::Craft, SkillType::EscapeArtist,
        SkillType::Disguise, SkillType::Ride, SkillType::KnowledgeArcana, SkillType::KnowledgeDungeoneering,
        SkillType::KnowledgeEngineering, SkillType::KnowledgeGeography, SkillType::KnowledgeHistory,
        SkillType::KnowledgeLocal, SkillType::KnowledgeNature, SkillType::KnowledgeNobility,
        SkillType::KnowledgePlanes, SkillType::KnowledgeReligion, SkillType::Diplomacy, SkillType::DisableDevice,
        SkillType::Stealth, SkillType::Heal, SkillType::Intimidate, SkillType::Perform, SkillType::SenseMotive,
        SkillType::Linguistics, SkillType::Swim, SkillType::Perception, SkillType::Profession, SkillType::Bluff,
        SkillType::SleightOfHand, SkillType::Spellcraft, SkillType::Climb, SkillType::Survival,
        SkillType::UseMagicDevice, SkillType::Appraise, SkillType::Fly
    };
    constexpr std::array SavingThrowNames{
        std::pair{"fortitude", SavingThrowType::Fortitude},
        std::pair{"reflex", SavingThrowType::Reflex},
        std::pair{"will", SavingThrowType::Will}
    };

    void requireArray(const Json &json)
    {
        if (!json.is_array()) throw std::invalid_argument("class collection must be a JSON array");
    }

    int integer(const Json &json)
    {
        if (!json.is_number_integer() || json < std::numeric_limits<int>::min() || json > std::numeric_limits<int>::max())
            throw std::invalid_argument("class numeric field must be an integer within range");
        return json.get<int>();
    }

    std::string text(const Json &json)
    {
        return normalize(json.get<std::string>());
    }

    void registerId(std::set<std::string> &ids, const std::string &id)
    {
        if (id.empty() || !ids.insert(id).second) throw std::invalid_argument("blank or duplicate class definition ID: " + id);
    }

    struct ParseState
    {
        std::set<std::string> acquisitions;
        std::map<std::string, std::vector<std::string>> dependencies;
        std::vector<std::pair<std::string, std::string>> replacements;
    };

    std::vector<ClassFeatureProgressionDefinition> progressionFromJson(const Json &json, ParseState &state, const std::string &parent)
    {
        requireArray(json);
        std::vector<ClassFeatureProgressionDefinition> result;
        for (const auto &entry : json)
        {
            ClassFeatureProgressionDefinition acquisition{.id = text(entry.at("id")), .level = integer(entry.at("level"))};
            registerId(state.acquisitions, acquisition.id);
            if (acquisition.level <= 0) throw std::invalid_argument("class acquisition level must be positive");
            if (!parent.empty()) state.dependencies[acquisition.id].push_back(parent);
            const auto effects = entry.value("effects", Json::array());
            requireArray(effects);
            std::set<std::string> effectIds;
            for (const auto &value : effects)
            {
                auto effect = effectDefinitionFromJson(value);
                registerId(effectIds, normalize(effectId(effect)));
                acquisition.effects.push_back(std::move(effect));
            }
            const auto choices = entry.value("choices", Json::array());
            requireArray(choices);
            std::set<std::string> choiceIds;
            for (const auto &value : choices)
            {
                const auto &options = value.at("options");
                requireArray(options);
                const int count = integer(value.at("selectionCount"));
                if (count <= 0 || static_cast<std::size_t>(count) > options.size())
                    throw std::invalid_argument("class choice selection count must be positive and within the option count");
                ClassChoiceDefinition choice{.id = text(value.at("id")), .prompt = text(value.at("prompt")),
                    .selectionCount = static_cast<std::size_t>(count), .options = {},
                    .blocksAdvancement = value.value("blocksAdvancement", true)};
                registerId(choiceIds, choice.id);
                std::set<std::string> optionIds;
                for (const auto &optionJson : options)
                {
                    ClassChoiceOptionDefinition option{.id = text(optionJson.at("id")), .name = text(optionJson.at("name")),
                        .description = text(optionJson.at("description")),
                        .progression = progressionFromJson(optionJson.value("progression", Json::array()), state, acquisition.id)};
                    registerId(optionIds, option.id);
                    choice.options.push_back(std::move(option));
                }
                acquisition.choices.push_back(std::move(choice));
            }
            const auto replaces = entry.value("replaces", Json::array());
            requireArray(replaces);
            std::set<std::string> replacementIds;
            for (const auto &value : replaces)
            {
                auto target = text(value);
                registerId(replacementIds, target);
                state.replacements.emplace_back(acquisition.id, target);
                acquisition.replaces.push_back(std::move(target));
            }
            result.push_back(std::move(acquisition));
        }
        return result;
    }

    void validateDependencies(ParseState &state)
    {
        for (const auto &[source, target] : state.replacements)
        {
            if (!state.acquisitions.contains(target)) throw std::invalid_argument("unknown class acquisition replacement: " + target);
            state.dependencies[target].push_back(source);
        }
        std::set<std::string> visiting, visited;
        std::function<void(const std::string &)> visit = [&](const std::string &id)
        {
            if (visited.contains(id)) return;
            if (!visiting.insert(id).second) throw std::invalid_argument("cyclic class acquisition dependencies");
            for (const auto &dependency : state.dependencies[id]) visit(dependency);
            visiting.erase(id);
            visited.insert(id);
        };
        for (const auto &id : state.acquisitions) visit(id);
    }

    Json progressionToJson(const std::vector<ClassFeatureProgressionDefinition> &progression)
    {
        Json result = Json::array();
        for (const auto &acquisition : progression)
        {
            Json effects = Json::array(), choices = Json::array();
            for (const auto &effect : acquisition.effects) effects.push_back(effectDefinitionToJson(effect));
            for (const auto &choice : acquisition.choices)
            {
                Json options = Json::array();
                for (const auto &option : choice.options)
                    options.push_back({{"id", option.id}, {"name", option.name}, {"description", option.description},
                        {"progression", progressionToJson(option.progression)}});
                choices.push_back({{"id", choice.id}, {"prompt", choice.prompt}, {"selectionCount", choice.selectionCount},
                    {"blocksAdvancement", choice.blocksAdvancement}, {"options", std::move(options)}});
            }
            result.push_back({{"id", acquisition.id}, {"level", acquisition.level}, {"effects", std::move(effects)},
                {"choices", std::move(choices)}, {"replaces", acquisition.replaces}});
        }
        return result;
    }
}

namespace golarion
{
    ArchetypeDefinition archetypeDefinitionFromJson(const nlohmann::json &json)
    {
        try
        {
            ArchetypeDefinition result{.id = text(json.at("id")), .classType = characterClassFromId(text(json.at("classType"))),
                .name = text(json.at("name")), .description = text(json.at("description"))};
            if (result.id.empty() || result.name.empty()) throw std::invalid_argument("archetype ID and name must not be blank");
            const auto impacts = json.value("affectedFeatures", Json::array());
            requireArray(impacts);
            std::set<std::string> impactIds;
            for (const auto &value : impacts)
            {
                const auto id = text(value.at("featureId"));
                registerId(impactIds, id);
                const auto type = text(value.at("type"));
                if (type != "alter" && type != "replace") throw std::invalid_argument("unknown archetype feature change type");
                result.affectedFeatures.push_back({id, type == "alter" ? ArchetypeFeatureChangeType::Alter : ArchetypeFeatureChangeType::Replace});
            }
            auto readIds = [&](const char *key, std::vector<std::string> &destination)
            {
                const auto values = json.value(key, Json::array());
                requireArray(values);
                std::set<std::string> ids;
                for (const auto &value : values)
                {
                    auto id = text(value);
                    registerId(ids, id);
                    destination.push_back(std::move(id));
                }
            };
            readIds("removedFeatureIds", result.removedFeatureIds);
            readIds("removedAcquisitionIds", result.removedAcquisitionIds);
            for (const auto &id : result.removedFeatureIds)
                if (!impactIds.contains(id)) throw std::invalid_argument("undeclared archetype feature removal");
            for (const auto &impact : result.affectedFeatures)
                if (impact.type == ArchetypeFeatureChangeType::Replace && std::find(result.removedFeatureIds.begin(), result.removedFeatureIds.end(), impact.featureId) == result.removedFeatureIds.end())
                    throw std::invalid_argument("archetype replacement requires whole-feature removal");
            const auto features = json.value("addedFeatures", Json::array());
            requireArray(features);
            ParseState state;
            std::set<std::string> featureIds;
            for (const auto &value : features)
            {
                ClassFeatureDefinition feature{.id = text(value.at("id")), .name = text(value.at("name")), .description = text(value.at("description")),
                    .progression = progressionFromJson(value.value("progression", Json::array()), state, {})};
                registerId(featureIds, feature.id);
                result.addedFeatures.push_back(std::move(feature));
            }
            validateDependencies(state);
            return result;
        }
        catch (const Json::exception &error)
        {
            throw std::invalid_argument("invalid archetype definition JSON: " + std::string(error.what()));
        }
    }

    nlohmann::json archetypeDefinitionToJson(const ArchetypeDefinition &definition)
    {
        Json impacts = Json::array(), features = Json::array();
        for (const auto &impact : definition.affectedFeatures)
        {
            std::string type;
            switch (impact.type)
            {
                case ArchetypeFeatureChangeType::Alter: type = "alter"; break;
                case ArchetypeFeatureChangeType::Replace: type = "replace"; break;
                default: throw std::invalid_argument("unknown archetype feature change type");
            }
            impacts.push_back({{"featureId", impact.featureId}, {"type", type}});
        }
        for (const auto &feature : definition.addedFeatures)
            features.push_back({{"id", feature.id}, {"name", feature.name}, {"description", feature.description}, {"progression", progressionToJson(feature.progression)}});
        Json result{{"id", definition.id}, {"classType", characterClassId(definition.classType)}, {"name", definition.name}, {"description", definition.description},
            {"affectedFeatures", std::move(impacts)}, {"removedFeatureIds", definition.removedFeatureIds},
            {"removedAcquisitionIds", definition.removedAcquisitionIds}, {"addedFeatures", std::move(features)}};
        (void)archetypeDefinitionFromJson(result);
        return result;
    }

    ClassDefinition classDefinitionFromJson(const nlohmann::json &json)
    {
        try
        {
            ClassDefinition result{.type = characterClassFromId(text(json.at("type"))), .name = text(json.at("name")),
                .description = text(json.at("description")), .hitDie = static_cast<ClassHitDie>(integer(json.at("hitDie"))),
                .skillRanksPerLevel = integer(json.at("skillRanksPerLevel")), .classSkills = {}, .progression = {}, .features = {}};
            switch (result.hitDie)
            {
                case ClassHitDie::D6: case ClassHitDie::D8: case ClassHitDie::D10: case ClassHitDie::D12: break;
                default: throw std::invalid_argument("unknown class hit die");
            }
            if (result.skillRanksPerLevel < 0) throw std::invalid_argument("class skill ranks must not be negative");
            const auto &skills = json.at("classSkills");
            requireArray(skills);
            for (const auto &value : skills)
            {
                const auto name = text(value);
                bool found = false;
                for (auto type : SkillTypes)
                    if (resourceName(type) == name)
                    {
                        if (!result.classSkills.insert(type).second) throw std::invalid_argument("duplicate class skill: " + name);
                        found = true;
                        break;
                    }
                if (!found) throw std::invalid_argument("unknown class skill: " + name);
            }
            const auto &progression = json.at("progression");
            requireArray(progression);
            std::set<int> levels;
            for (const auto &value : progression)
            {
                ClassLevelDefinition row{.level = integer(value.at("level")), .baseAttackBonus = integer(value.at("baseAttackBonus")), .baseSavingThrows = {}};
                if (row.level <= 0 || row.baseAttackBonus < 0 || !levels.insert(row.level).second)
                    throw std::invalid_argument("invalid class level table");
                const auto &saves = value.at("baseSavingThrows");
                if (!saves.is_object() || saves.size() != SavingThrowNames.size()) throw std::invalid_argument("class level requires all three base saving throws");
                for (const auto &[name, type] : SavingThrowNames)
                {
                    const int bonus = integer(saves.at(name));
                    if (bonus < 0) throw std::invalid_argument("class base saving throw must not be negative");
                    row.baseSavingThrows.emplace(type, bonus);
                }
                result.progression.push_back(std::move(row));
            }
            const auto features = json.value("features", Json::array());
            requireArray(features);
            std::set<std::string> featureIds;
            ParseState state;
            for (const auto &value : features)
            {
                ClassFeatureDefinition feature{.id = text(value.at("id")), .name = text(value.at("name")),
                    .description = text(value.at("description")),
                    .progression = progressionFromJson(value.value("progression", Json::array()), state, {})};
                registerId(featureIds, feature.id);
                result.features.push_back(std::move(feature));
            }
            validateDependencies(state);
            return result;
        }
        catch (const Json::exception &error)
        {
            throw std::invalid_argument("invalid class definition JSON: " + std::string(error.what()));
        }
    }

    nlohmann::json classDefinitionToJson(const ClassDefinition &definition)
    {
        Json skills = Json::array(), progression = Json::array(), features = Json::array();
        for (auto skill : definition.classSkills) skills.push_back(resourceName(skill));
        for (const auto &row : definition.progression)
        {
            if (row.baseSavingThrows.size() != SavingThrowNames.size()) throw std::invalid_argument("class level requires all three base saving throws");
            Json saves = Json::object();
            for (const auto &[name, type] : SavingThrowNames)
            {
                if (!row.baseSavingThrows.contains(type)) throw std::invalid_argument("unknown or missing class base saving throw");
                saves[name] = row.baseSavingThrows.at(type);
            }
            progression.push_back({{"level", row.level}, {"baseAttackBonus", row.baseAttackBonus}, {"baseSavingThrows", std::move(saves)}});
        }
        for (const auto &feature : definition.features)
            features.push_back({{"id", feature.id}, {"name", feature.name}, {"description", feature.description},
                {"progression", progressionToJson(feature.progression)}});
        Json result{{"type", characterClassId(definition.type)}, {"name", definition.name}, {"description", definition.description},
            {"hitDie", static_cast<int>(definition.hitDie)}, {"skillRanksPerLevel", definition.skillRanksPerLevel},
            {"classSkills", std::move(skills)}, {"progression", std::move(progression)}, {"features", std::move(features)}};
        (void)classDefinitionFromJson(result);
        return result;
    }
}
