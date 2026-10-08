#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct FeatRequirementDefinition
    {
        std::string description;
        std::optional<std::string> expression = std::nullopt;
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

    struct FeatDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> types;
        std::vector<FeatRequirementDefinition> requirements{};
        std::vector<EffectDefinition> effects{};
        std::vector<FeatChoiceDefinition> choices{};
    };
}
