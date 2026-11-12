#pragma once

#include "golarion/character/enhancement_definition.hpp"

namespace golarion
{
    struct EnhancementGroupData
    {
        std::string id;
        std::string name;
        std::vector<EffectDefinition> enhancements;
        bool active = true;
        std::optional<std::string> description;
        std::optional<EnhancementOrigin> origin;
    };

    struct EnhancementsData
    {
        std::vector<EnhancementGroupData> groups;
    };
}
