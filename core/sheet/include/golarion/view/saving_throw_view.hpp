#pragma once

#include "golarion/character/saving_throw.hpp"
#include "golarion/view/contribution_set_view.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct SavingThrowAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
    };

    struct SavingThrowView
    {
        SavingThrowType type;
        int baseValue;
        ContributionSetView baseContributions;
        std::vector<SavingThrowAbilityOptionView> abilityOptions;
        ModifierSetView modifiers;
    };
}
