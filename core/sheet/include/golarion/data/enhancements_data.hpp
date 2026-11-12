#pragma once

#include "golarion/character/enhancement_definition.hpp"

namespace golarion
{
    struct EnhancementData
    {
        EnhancementEffectDefinition definition;
        bool active = true;
    };

    struct EnhancementGroupData
    {
        std::string id;
        std::string name;
        std::vector<EnhancementData> enhancements;
    };

    struct EnhancementsData
    {
        std::vector<EnhancementGroupData> groups;
    };
}
