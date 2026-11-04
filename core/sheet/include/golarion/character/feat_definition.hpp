#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct FeatDependencyDefinition
    {
        std::string id;
        // Prerequisite choice ID -> current feat choice ID.
        std::map<std::string, std::string> matchingChoices;
    };

    struct FeatRequirementDefinition
    {
        std::string description;
        std::optional<std::string> expression = std::nullopt;
        std::optional<FeatDependencyDefinition> feat = std::nullopt;
    };

    struct FeatChoiceOptionDefinition
    {
        std::string id;
        std::string name;
        std::vector<FeatRequirementDefinition> requirements{};
        std::vector<EffectDefinition> effects{};
    };

    struct FeatChoiceDefinition
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<FeatChoiceOptionDefinition> options;
    };

    enum class FeatRepeatability { Once, DifferentChoices, Unlimited };

    struct FeatDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> types;
        std::vector<FeatRequirementDefinition> requirements{};
        std::vector<EffectDefinition> effects{};
        std::vector<FeatChoiceDefinition> choices{};
        FeatRepeatability repeatability = FeatRepeatability::Once;
    };
}
