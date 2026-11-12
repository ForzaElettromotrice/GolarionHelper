#pragma once

#include "golarion/data/enhancements_data.hpp"

namespace golarion
{
    struct EnhancementView
    {
        EffectDefinition definition;
        // Only numeric contributions/modifiers have a raw expression value.
        std::optional<int> resolvedValue;
    };

    struct EnhancementGroupView
    {
        std::string id;
        std::string name;
        std::vector<EnhancementView> enhancements;
        bool active;
        std::optional<std::string> description;
        std::optional<EnhancementOrigin> origin;
    };

    struct EnhancementsView
    {
        std::vector<EnhancementGroupView> groups;
    };
}
