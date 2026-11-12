#pragma once

#include "golarion/effect/effect_definition.hpp"

namespace golarion
{
    using EnhancementEffectDefinition = std::variant<ContributionEffectDefinition, ModifierEffectDefinition>;

    struct EnhancementDefinition
    {
        EnhancementEffectDefinition definition;
        bool active = true;
    };

    struct EnhancementGroupDefinition
    {
        std::string id;
        std::string name;
        std::vector<EnhancementDefinition> enhancements{};
    };
}
