#pragma once

#include "golarion/character/attack_distance.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct AttackDistanceAdjustmentView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AttackDistanceAdjustmentType type;
        std::string expression;
        int resolvedValue;
        std::optional<std::string> condition;
    };

    struct ConditionalAttackDistanceValueView
    {
        std::string condition;
        int value;
    };

    struct AttackDistanceValueView
    {
        std::string resourceName;
        int baseValue;
        int effectiveValue;
        std::vector<ConditionalAttackDistanceValueView> conditionalValues;
        ModifierSetView modifiers;
        std::vector<AttackDistanceAdjustmentView> adjustments;
    };

    struct AttackReachView
    {
        AttackDistanceValueView minimumUnits;
        AttackDistanceValueView maximumUnits;
    };

    struct AttackRangeView
    {
        AttackDistanceValueView incrementUnits;
        AttackDistanceValueView maximumIncrements;
        AttackDistanceValueView penaltyPerAdditionalIncrement;
    };
}
