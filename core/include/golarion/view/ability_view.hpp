#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/view/modifier_set_view.hpp"

namespace golarion
{
    struct AbilityView
    {
        AbilityType type;
        int baseValue;
        int totalValue;
        int modifier;
        ModifierSetView modifiers;
    };
}
