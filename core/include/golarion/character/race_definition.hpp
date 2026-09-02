#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace golarion
{
    struct RacialChoiceOptionDefinition
    {
        std::string id;
        std::string name;
        std::vector<EffectDefinition> effects{};
    };

    struct RacialChoiceDefinition
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<RacialChoiceOptionDefinition> options;
    };

    struct RacialElementDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<EffectDefinition> effects{};
        std::vector<RacialChoiceDefinition> choices{};
        std::vector<std::string> replaces{};
    };

    struct RaceDefinition
    {
        std::string id;
        std::string name;
        std::vector<RacialElementDefinition> qualities;
        std::vector<RacialElementDefinition> standardFeatures;
        std::vector<RacialElementDefinition> alternateFeatures;
    };
}
