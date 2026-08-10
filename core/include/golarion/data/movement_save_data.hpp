#pragma once

#include "golarion/character/movement.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct MovementGrantSaveData
    {
        std::string id;
        std::string source;
        MovementType type;
        std::string baseSpeedExpression;
        std::optional<Maneuverability> maneuverability;
        bool affectedByArmor;
        bool affectedByLoad;
    };

    struct MovementAdjustmentSaveData
    {
        std::string id;
        std::string source;
        std::string description;
        MovementAdjustmentType type;
        MovementSelector selector;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
    };

}
