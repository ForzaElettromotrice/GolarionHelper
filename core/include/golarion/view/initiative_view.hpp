#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/view/modifier_set_view.hpp"

namespace golarion
{
    struct InitiativeView
    {
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
        ModifierSetView modifiers;
    };
}
