#pragma once

#include "golarion/character/armor_class.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct ArmorClassValueView
    {
        ArmorClassType type;
        int appliedAbilityModifier;
        int totalValue;
        ModifierSetView modifiers;
    };

    struct MaximumDexterityLimitView
    {
        std::string id;
        std::string source;
        MaximumDexterityLimitType type;
        int baseValue;
        int effectiveValue;
        ModifierSetView modifiers;
    };

    struct ArmorClassAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        std::vector<ArmorClassValueView> values;
    };

    struct ArmorClassAbilitySuppressionView
    {
        std::string id;
        std::string source;
    };

    struct ArmorClassView
    {
        std::optional<int> maximumDexterityBonus;
        std::vector<MaximumDexterityLimitView> maximumDexterityLimits;
        bool abilityBonusSuppressed;
        std::vector<ArmorClassAbilitySuppressionView> abilitySuppressions;
        std::vector<ArmorClassAbilityOptionView> abilityOptions;
    };
}
