#include "golarion/character/class_option_catalog_json.hpp"

#include "golarion/character/class_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
    using Json = nlohmann::json;
    using namespace golarion;

    void array(const Json &value)
    {
        if (!value.is_array()) throw std::invalid_argument("class option collection must be an array");
    }

    std::string id(const Json &value)
    {
        const auto result = normalize(value.get<std::string>());
        if (result.empty()) throw std::invalid_argument("class option ID must not be blank");
        return result;
    }

    void keys(const Json &value, std::initializer_list<std::string_view> allowed)
    {
        if (!value.is_object()) throw std::invalid_argument("class option record must be an object");
        for (const auto &[key, unused] : value.items())
            if (std::find(allowed.begin(), allowed.end(), key) == allowed.end())
                throw std::invalid_argument("unknown class option field: " + key);
    }

    void walk(Json &stages, const std::function<void(Json &)> &visit)
    {
        array(stages);
        for (auto &stage : stages)
        {
            visit(stage);
            if (stage.contains("choices"))
                for (auto &choice : stage.at("choices"))
                    for (auto &option : choice.at("options"))
                        if (option.contains("progression")) walk(option.at("progression"), visit);
        }
    }

    void expandChoice(Json &choice, const std::map<std::string, Json> &sets, std::set<std::string> &visiting)
    {
        keys(choice, {"id", "prompt", "selectionCount", "blocksAdvancement", "optionSet", "options"});
        std::optional<std::string> referenced;
        if (choice.contains("optionSet"))
        {
            if (choice.contains("options")) throw std::invalid_argument("class choice cannot combine inline options and an option set");
            referenced = id(choice.at("optionSet"));
            if (!sets.contains(*referenced)) throw std::invalid_argument("unknown class option set: " + *referenced);
            if (!visiting.insert(*referenced).second) throw std::invalid_argument("cyclic class option set references");
            choice["options"] = sets.at(*referenced).at("options");
            choice.erase("optionSet");
        }
        array(choice.at("options"));
        std::set<std::string> optionIds;
        for (auto &option : choice.at("options"))
        {
            keys(option, {"id", "name", "description", "progression"});
            if (!optionIds.insert(id(option.at("id"))).second) throw std::invalid_argument("duplicate class catalog option ID");
            if (!option.contains("progression")) option["progression"] = Json::array();
            array(option.at("progression"));
            for (auto &stage : option.at("progression"))
                if (stage.contains("choices"))
                    for (auto &nested : stage.at("choices")) expandChoice(nested, sets, visiting);
        }
        if (referenced) visiting.erase(*referenced);
    }

    void namespaceChoice(Json &choice, const std::string &prefix)
    {
        for (auto &option : choice.at("options"))
        {
            const auto optionPrefix = prefix + ".option." + id(option.at("id")) + ".";
            std::map<std::string, std::string> stages;
            for (auto &stage : option.at("progression"))
            {
                const auto localId = id(stage.at("id"));
                if (!stages.emplace(localId, optionPrefix + localId).second) throw std::invalid_argument("duplicate class option acquisition ID");
            }
            for (auto &stage : option.at("progression"))
            {
                stage["id"] = stages.at(id(stage.at("id")));
                if (stage.contains("replaces"))
                    for (auto &target : stage.at("replaces"))
                    {
                        const auto localId = id(target);
                        if (!stages.contains(localId)) throw std::invalid_argument("class option replaces an acquisition outside its own progression");
                        target = stages.at(localId);
                    }
                if (stage.contains("choices"))
                    for (auto &nested : stage.at("choices")) namespaceChoice(nested, stage.at("id").get<std::string>() + ".choice." + id(nested.at("id")));
            }
        }
    }
}

namespace golarion
{
    std::vector<ClassDefinition> classDefinitionsWithOptionsFromJson(const nlohmann::json &definitions, const nlohmann::json &optionCatalogs)
    {
        try
        {
            array(definitions);
            array(optionCatalogs);
            std::map<CharacterClassType, Json> classes;
            std::map<std::string, Json> sets;
            std::map<CharacterClassType, std::map<std::string, std::set<std::string>>> originalAcquisitions;
            std::map<CharacterClassType, std::set<std::string>> manualAcquisitions;
            for (const auto &entry : definitions)
            {
                const auto definition = classDefinitionFromJson(entry);
                if (!classes.emplace(definition.type, classDefinitionToJson(definition)).second) throw std::invalid_argument("duplicate class definition in option catalog input");
                for (auto &feature : classes.at(definition.type).at("features"))
                    walk(feature.at("progression"), [&](Json &stage)
                    {
                        originalAcquisitions[definition.type][id(feature.at("id"))].insert(id(stage.at("id")));
                        const auto effects = stage.value("effects", Json::array());
                        if (stage.value("choices", Json::array()).empty() && std::all_of(effects.begin(), effects.end(), [](const auto &effect) { return effect.at("type") == "reminder"; }))
                            manualAcquisitions[definition.type].insert(id(stage.at("id")));
                    });
            }
            for (const auto &catalog : optionCatalogs)
            {
                keys(catalog, {"optionSets", "bindings"});
                array(catalog.at("optionSets"));
                array(catalog.at("bindings"));
                for (const auto &set : catalog.at("optionSets"))
                {
                    keys(set, {"id", "name", "description", "options"});
                    const auto setId = id(set.at("id"));
                    array(set.at("options"));
                    if (set.at("options").empty() || !sets.emplace(setId, set).second) throw std::invalid_argument("empty or duplicate class option set");
                }
            }
            // Validate even unused catalog sets, including reference cycles.
            for (const auto &[setId, unused] : sets)
            {
                Json choice{{"id", "catalog"}, {"prompt", "Catalog validation"}, {"selectionCount", 1}, {"optionSet", setId}};
                std::set<std::string> visiting;
                expandChoice(choice, sets, visiting);
                namespaceChoice(choice, "catalog");
                const Json validation{{"type", "wizard"}, {"name", "Catalog validation"}, {"description", "Catalog validation"}, {"hitDie", 6},
                    {"skillRanksPerLevel", 0}, {"classSkills", Json::array()}, {"progression", Json::array()},
                    {"features", Json::array({{{"id", "catalog"}, {"name", "Catalog"}, {"description", "Catalog validation"},
                        {"progression", Json::array({{{"id", "root"}, {"level", 1}, {"choices", Json::array({choice})}}})}}})}};
                (void)classDefinitionFromJson(validation);
            }
            std::set<std::string> bindingIds;
            for (const auto &catalog : optionCatalogs)
                for (const auto &binding : catalog.at("bindings"))
                {
                    keys(binding, {"id", "classType", "featureId", "acquisitionIds", "choice", "retainEarlierAcquisitions"});
                    if (!bindingIds.insert(id(binding.at("id"))).second) throw std::invalid_argument("duplicate class option binding");
                    const auto type = characterClassFromId(id(binding.at("classType")));
                    if (!classes.contains(type)) throw std::invalid_argument("class option binding targets an unregistered class");
                    const auto featureId = id(binding.at("featureId"));
                    if (!originalAcquisitions[type].contains(featureId)) throw std::invalid_argument("class option binding targets an unknown feature");
                    const auto &targets = binding.at("acquisitionIds");
                    array(targets);
                    if (targets.empty()) throw std::invalid_argument("class option binding needs acquisition targets");
                    auto choice = binding.at("choice");
                    std::set<std::string> visiting, targetIds;
                    expandChoice(choice, sets, visiting);
                    const auto choiceId = id(choice.at("id"));
                    const bool retain = binding.value("retainEarlierAcquisitions", false);
                    for (const auto &target : targets)
                        if (!targetIds.insert(id(target)).second || !originalAcquisitions[type][featureId].contains(id(target)))
                            throw std::invalid_argument("duplicate or unknown class option acquisition target");
                    for (const auto &target : targets)
                    {
                        const auto targetId = id(target);
                        if (retain && !manualAcquisitions[type].contains(targetId))
                            throw std::invalid_argument("cannot retain upgraded acquisitions with automatic effects or existing choices");
                        auto instance = choice;
                        namespaceChoice(instance, targetId + ".choice." + choiceId);
                        for (auto &feature : classes.at(type).at("features"))
                            if (feature.at("id") == featureId)
                                walk(feature.at("progression"), [&](Json &stage)
                                {
                                    if (stage.at("id") == targetId)
                                    {
                                        if (retain && stage.contains("replaces"))
                                            std::erase_if(stage.at("replaces").get_ref<Json::array_t &>(), [&](const Json &value) { return targetIds.contains(id(value)); });
                                        if (!stage.contains("choices")) stage["choices"] = Json::array();
                                        stage.at("choices").push_back(instance);
                                    }
                                });
                    }
                }
            std::vector<ClassDefinition> result;
            result.reserve(classes.size());
            for (const auto &[type, entry] : classes) result.push_back(classDefinitionFromJson(entry));
            return result;
        }
        catch (const Json::exception &error)
        {
            throw std::invalid_argument("invalid class option catalog JSON: " + std::string(error.what()));
        }
    }
}
