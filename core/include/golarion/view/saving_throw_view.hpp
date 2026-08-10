#pragma once

#include "golarion/character/saving_throw.hpp"
#include "golarion/view/modifier_set_view.hpp"

namespace golarion
{
    struct SavingThrowView
    {
        SavingThrowType type;
        int baseValue;
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
        ModifierSetView modifiers;
    };
}
