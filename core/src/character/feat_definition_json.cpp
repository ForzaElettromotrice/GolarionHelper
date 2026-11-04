#include "golarion/character/feat_definition_json.hpp"

#include "golarion/effect/effect_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <set>
#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    void requireArray(const Json &json, const std::string &field)
    {
        if (!json.is_array())
        {
            throw std::invalid_argument("feat " + field + " must be a JSON array");
        }
    }

    void registerId(std::set<std::string> &ids, const std::string &id, const std::string &field)
    {
        if (!ids.insert(id).second)
        {
            throw std::invalid_argument("duplicate feat " + field + " ID: " + id);
        }
    }

    std::vector<golarion::FeatRequirementDefinition> requirementsFromJson(const Json &json)
    {
        requireArray(json, "requirements");
        std::vector<golarion::FeatRequirementDefinition> requirements;
        for (const Json &entry : json)
        {
            golarion::FeatRequirementDefinition requirement{
                .description = golarion::normalize(entry.at("description").get<std::string>())
            };
            if (entry.contains("expression"))
            {
                requirement.expression = golarion::normalize(entry.at("expression").get<std::string>());
            }
            if (entry.contains("feat"))
            {
                if (requirement.expression) throw std::invalid_argument("feat requirement cannot combine expression and dependency");
                const auto &dependency = entry.at("feat");
                golarion::FeatDependencyDefinition feat{golarion::normalize(dependency.at("id").get<std::string>()), {}};
                const auto bindings = dependency.value("matchingChoices", Json::object());
                if (!bindings.is_object()) throw std::invalid_argument("feat matchingChoices must be an object");
                for (auto it = bindings.begin(); it != bindings.end(); ++it)
                    if (!feat.matchingChoices.emplace(golarion::normalize(it.key()), golarion::normalize(it.value().get<std::string>())).second)
                        throw std::invalid_argument("duplicate normalized feat dependency choice");
                requirement.feat = std::move(feat);
            }
            requirements.push_back(std::move(requirement));
        }
        return requirements;
    }

    std::vector<golarion::EffectDefinition> effectsFromJson(const Json &json)
    {
        requireArray(json, "effects");
        std::vector<golarion::EffectDefinition> effects;
        std::set<std::string> ids;
        for (const Json &entry : json)
        {
            auto effect = golarion::effectDefinitionFromJson(entry);
            registerId(ids, golarion::effectId(effect), "effect");
            effects.push_back(std::move(effect));
        }
        return effects;
    }

    std::vector<golarion::FeatChoiceDefinition> choicesFromJson(const Json &json)
    {
        requireArray(json, "choices");
        std::vector<golarion::FeatChoiceDefinition> choices;
        std::set<std::string> choiceIds;
        for (const Json &entry : json)
        {
            const std::string id = golarion::normalize(entry.at("id").get<std::string>());
            registerId(choiceIds, id, "choice");
            const Json &optionsJson = entry.at("options");
            requireArray(optionsJson, "choice options");
            const Json &count = entry.at("selectionCount");
            if (!count.is_number_integer() || count <= 0 || count > optionsJson.size())
            {
                throw std::invalid_argument("feat choice selection count must be a positive integer within the option count: " + id);
            }
            golarion::FeatChoiceDefinition choice{
                .id = id,
                .prompt = golarion::normalize(entry.at("prompt").get<std::string>()),
                .selectionCount = count.get<std::size_t>(),
                .options = {}
            };
            std::set<std::string> optionIds;
            for (const Json &option : optionsJson)
            {
                const std::string optionId = golarion::normalize(option.at("id").get<std::string>());
                registerId(optionIds, optionId, "choice option");
                choice.options.push_back(golarion::FeatChoiceOptionDefinition{
                    .id = optionId,
                    .name = golarion::normalize(option.at("name").get<std::string>()),
                    .requirements = requirementsFromJson(option.value("requirements", Json::array())),
                    .effects = effectsFromJson(option.value("effects", Json::array()))
                });
            }
            choices.push_back(std::move(choice));
        }
        return choices;
    }
}

namespace golarion
{
    FeatDefinition featDefinitionFromJson(const nlohmann::json &json)
    {
        try
        {
            if (!json.is_object())
            {
                throw std::invalid_argument("feat definition must be a JSON object");
            }
            const Json &typesJson = json.at("types");
            requireArray(typesJson, "types");
            std::vector<std::string> types;
            std::set<std::string> typeIds;
            for (const Json &entry : typesJson)
            {
                const std::string type = normalize(entry.get<std::string>());
                registerId(typeIds, type, "type");
                types.push_back(type);
            }
            const auto repeatabilityName = json.value("repeatability", std::string("once"));
            const auto repeatability = repeatabilityName == "once" ? FeatRepeatability::Once
                : repeatabilityName == "differentChoices" ? FeatRepeatability::DifferentChoices
                : repeatabilityName == "unlimited" ? FeatRepeatability::Unlimited
                : throw std::invalid_argument("unknown feat repeatability: " + repeatabilityName);
            auto choices = choicesFromJson(json.value("choices", Json::array()));
            if (repeatability == FeatRepeatability::DifferentChoices && choices.empty())
                throw std::invalid_argument("differentChoices repeatability requires choices");
            return FeatDefinition{
                .id = normalize(json.at("id").get<std::string>()),
                .name = normalize(json.at("name").get<std::string>()),
                .description = normalize(json.at("description").get<std::string>()),
                .types = std::move(types),
                .requirements = requirementsFromJson(json.value("requirements", Json::array())),
                .effects = effectsFromJson(json.value("effects", Json::array())),
                .choices = std::move(choices),
                .repeatability = repeatability
            };
        }
        catch (const Json::exception &exception)
        {
            throw std::invalid_argument("invalid feat definition JSON: " + std::string(exception.what()));
        }
    }
}
