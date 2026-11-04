#pragma once

#include "golarion/character/movement.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct MovementAdjustmentView
    {
        std::string id;
        std::string source;
        std::string description;
        MovementAdjustmentType type;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
        std::optional<int> resolvedValue;
    };

    struct RunAdjustmentView
    {
        std::string id;
        std::string source;
        std::string description;
        RunAdjustmentType type;
        std::string stackingGroup;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
        std::optional<int> resolvedValue;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct RunView
    {
        bool supported;
        bool usable;
        int baseMultiplier;
        std::optional<int> effectiveMultiplier;
        std::optional<int> distanceUnits;
        std::vector<std::string> notUsableReasons;
        std::vector<RunAdjustmentView> adjustments;
    };

    struct MovementGrantView
    {
        std::string id;
        std::string source;
        MovementType type;
        std::string resourceName;
        std::string baseSpeedExpression;
        int baseUnits;
        int modifiedBaseUnits;
        int effectiveUnits;
        std::optional<Maneuverability> maneuverability;
        bool affectedByArmor;
        bool affectedByLoad;
        bool usable;
        RunView run;
        ModifierSetView modifiers;
        std::vector<MovementAdjustmentView> adjustments;
    };

    struct MovementView
    {
        std::vector<MovementGrantView> grants;
    };
}
