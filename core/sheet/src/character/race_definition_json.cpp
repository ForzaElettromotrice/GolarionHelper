#include "golarion/character/race_definition_json.hpp"

#include "golarion/effect/effect_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using Json = nlohmann::json;

    std::vector<golarion::EffectDefinition> effectsFromJson(const Json &json, std::string_view ownerId)
    {
        if (!json.is_array())
        {
            throw std::invalid_argument("racial effects must be a JSON array: " + std::string(ownerId));
        }

        std::vector<golarion::EffectDefinition> effects;
        effects.reserve(json.size());
        for (const Json &effectJson : json)
        {
            golarion::EffectDefinition effect = golarion::effectDefinitionFromJson(effectJson);
            const std::string id = golarion::effectId(effect);
            if (std::ranges::any_of(effects, [&id](const golarion::EffectDefinition &registeredEffect)
            {
                return golarion::effectId(registeredEffect) == id;
            }))
            {
                throw std::invalid_argument("racial effects must have unique IDs within " + std::string(ownerId) + ": " + id);
            }
            effects.push_back(std::move(effect));
        }
        return effects;
    }

    std::vector<golarion::RacialChoiceDefinition> choicesFromJson(const Json &json, std::string_view elementId)
    {
        if (!json.is_array())
        {
            throw std::invalid_argument("racial choices must be a JSON array: " + std::string(elementId));
        }

        std::vector<golarion::RacialChoiceDefinition> choices;
        choices.reserve(json.size());
        for (const Json &choiceJson : json)
        {
            const std::string choiceId = golarion::normalize(choiceJson.at("id").get<std::string>());
            if (std::ranges::any_of(choices, [&choiceId](const golarion::RacialChoiceDefinition &choice)
            {
                return choice.id == choiceId;
            }))
            {
                throw std::invalid_argument("racial choice is duplicated for " + std::string(elementId) + ": " + choiceId);
            }

            const int selectionCount = choiceJson.at("selectionCount").get<int>();
            if (selectionCount < 1)
            {
                throw std::invalid_argument("racial choice selection count must be positive: " + choiceId);
            }

            const Json &optionsJson = choiceJson.at("options");
            if (!optionsJson.is_array())
            {
                throw std::invalid_argument("racial choice options must be a JSON array: " + choiceId);
            }
            std::vector<golarion::RacialChoiceOptionDefinition> options;
            options.reserve(optionsJson.size());
            for (const Json &optionJson : optionsJson)
            {
                const std::string optionId = golarion::normalize(optionJson.at("id").get<std::string>());
                if (std::ranges::any_of(options, [&optionId](const golarion::RacialChoiceOptionDefinition &option)
                {
                    return option.id == optionId;
                }))
                {
                    throw std::invalid_argument("racial choice option is duplicated for " + choiceId + ": " + optionId);
                }
                options.push_back(golarion::RacialChoiceOptionDefinition{
                    .id = optionId,
                    .name = golarion::normalize(optionJson.at("name").get<std::string>()),
                    .effects = effectsFromJson(optionJson.value("effects", Json::array()), std::string(elementId) + "." + choiceId + "." + optionId)
                });
            }
            if (static_cast<std::size_t>(selectionCount) > options.size())
            {
                throw std::invalid_argument("racial choice requires more selections than available options: " + choiceId);
            }
            choices.push_back(golarion::RacialChoiceDefinition{
                .id = choiceId,
                .prompt = golarion::normalize(choiceJson.at("prompt").get<std::string>()),
                .selectionCount = static_cast<std::size_t>(selectionCount),
                .options = std::move(options)
            });
        }
        return choices;
    }

    std::vector<std::string> replacementsFromJson(const Json &json, std::string_view elementId)
    {
        std::vector<std::string> replacements = json.value("replaces", std::vector<std::string>{});
        std::set<std::string> replacementIds;
        for (std::string &replacement : replacements)
        {
            replacement = golarion::normalize(replacement);
            if (!replacementIds.insert(replacement).second)
            {
                throw std::invalid_argument("racial element replacements must not contain duplicates: " + std::string(elementId));
            }
        }
        return replacements;
    }

    std::vector<golarion::RacialElementDefinition> elementsFromJson(const Json &json, std::string_view raceId, std::string_view category)
    {
        if (!json.is_array())
        {
            throw std::invalid_argument("race " + std::string(category) + " must be a JSON array: " + std::string(raceId));
        }

        std::vector<golarion::RacialElementDefinition> elements;
        elements.reserve(json.size());
        for (const Json &elementJson : json)
        {
            const std::string elementId = golarion::normalize(elementJson.at("id").get<std::string>());
            if (std::ranges::any_of(elements, [&elementId](const golarion::RacialElementDefinition &element)
            {
                return element.id == elementId;
            }))
            {
                throw std::invalid_argument("racial element is duplicated within " + std::string(category) + ": " + elementId);
            }
            elements.push_back(golarion::RacialElementDefinition{
                .id = elementId,
                .name = golarion::normalize(elementJson.at("name").get<std::string>()),
                .description = golarion::normalize(elementJson.at("description").get<std::string>()),
                .effects = effectsFromJson(elementJson.value("effects", Json::array()), elementId),
                .choices = choicesFromJson(elementJson.value("choices", Json::array()), elementId),
                .replaces = replacementsFromJson(elementJson, elementId)
            });
        }
        return elements;
    }

    void registerElementIds(const std::vector<golarion::RacialElementDefinition> &elements, std::set<std::string> &elementIds)
    {
        for (const golarion::RacialElementDefinition &element : elements)
        {
            if (!elementIds.insert(element.id).second)
            {
                throw std::invalid_argument("racial element ID is duplicated across categories: " + element.id);
            }
        }
    }
}

namespace golarion
{
    RaceDefinition raceDefinitionFromJson(const nlohmann::json &json)
    {
        if (!json.is_object())
        {
            throw std::invalid_argument("race definition must be a JSON object");
        }

        const std::string id = normalize(json.at("id").get<std::string>());
        RaceDefinition definition{
            .id = id,
            .name = normalize(json.at("name").get<std::string>()),
            .qualities = elementsFromJson(json.at("qualities"), id, "qualities"),
            .standardFeatures = elementsFromJson(json.at("standardFeatures"), id, "standard features"),
            .alternateFeatures = elementsFromJson(json.at("alternateFeatures"), id, "alternate features")
        };

        std::set<std::string> elementIds;
        registerElementIds(definition.qualities, elementIds);
        registerElementIds(definition.standardFeatures, elementIds);
        registerElementIds(definition.alternateFeatures, elementIds);

        std::set<std::string> baselineElementIds;
        for (const RacialElementDefinition &quality : definition.qualities)
        {
            baselineElementIds.insert(quality.id);
            if (!quality.replaces.empty())
            {
                throw std::invalid_argument("racial qualities cannot replace other elements: " + quality.id);
            }
        }
        for (const RacialElementDefinition &feature : definition.standardFeatures)
        {
            baselineElementIds.insert(feature.id);
            if (!feature.replaces.empty())
            {
                throw std::invalid_argument("standard racial features cannot replace other elements: " + feature.id);
            }
        }
        for (const RacialElementDefinition &feature : definition.alternateFeatures)
        {
            for (const std::string &replacement : feature.replaces)
            {
                if (!baselineElementIds.contains(replacement))
                {
                    throw std::invalid_argument("alternate racial feature replaces an unknown baseline element: " + feature.id + " -> " + replacement);
                }
            }
        }

        return definition;
    }
}
