#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace golarion
{
    struct ClassFeatureProgressionDefinition;

    struct ClassChoiceOptionDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        // Thresholds use the owning class level, not levels since selection.
        std::vector<ClassFeatureProgressionDefinition> progression{};
    };

    struct ClassChoiceDefinition
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<ClassChoiceOptionDefinition> options;
        bool blocksAdvancement = true;
    };

    struct ClassFeatureProgressionDefinition
    {
        // Stable acquisition ID, unique across the class definition.
        std::string id;
        int level;
        std::vector<EffectDefinition> effects{};
        std::vector<ClassChoiceDefinition> choices{};
        // Earlier acquisitions remain active unless explicitly replaced.
        // References acquisition IDs, not effect or feature IDs.
        std::vector<std::string> replaces{};
    };

    struct ClassFeatureDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<ClassFeatureProgressionDefinition> progression{};
    };
}
