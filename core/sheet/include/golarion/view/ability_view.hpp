#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/view/modifier_set_view.hpp"
#include "golarion/view/requirement_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct AbilityReplacementView
    {
        std::string id;
        std::string source;
        std::string expression;
        AbilityReplacementStage stage;
        int resolvedValue;
        bool active;
        bool applied;
        std::vector<RequirementView> requirements;
    };

    struct AbilityView
    {
        AbilityType type;
        int baseValue;
        int effectiveBaseValue;
        int modifiedValue;
        int totalValue;
        int modifier;
        ModifierSetView modifiers;
        std::vector<AbilityReplacementView> replacements;
        int checkTotal;
        ModifierSetView checkModifiers;
    };
}
