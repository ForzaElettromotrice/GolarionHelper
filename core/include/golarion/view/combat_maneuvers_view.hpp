#pragma once

#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct CombatManeuverAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
    };

    struct CombatManeuverBonusView
    {
        std::vector<CombatManeuverAbilityOptionView> abilityOptions;
        ModifierSetView modifiers;
    };

    struct CombatManeuverDefenseDexteritySuppressionView
    {
        std::string id;
        std::string source;
    };

    struct CombatManeuverDefenseView
    {
        int strengthModifier;
        int dexterityModifier;
        int appliedDexterityModifier;
        bool dexterityBonusSuppressed;
        std::vector<CombatManeuverDefenseDexteritySuppressionView> dexteritySuppressions;
        int totalValue;
        ModifierSetView modifiers;
    };

    struct CombatManeuverView
    {
        CombatManeuverType type;
        CombatManeuverBonusView bonus;
        CombatManeuverDefenseView defense;
    };

    struct CombatManeuversView
    {
        int baseAttackBonus;
        std::vector<CombatManeuverView> maneuvers;
    };
}
