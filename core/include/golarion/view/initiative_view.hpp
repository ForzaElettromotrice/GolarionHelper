#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct InitiativeAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
    };

    struct InitiativeView
    {
        std::vector<InitiativeAbilityOptionView> abilityOptions;
        ModifierSetView modifiers;
    };
}
