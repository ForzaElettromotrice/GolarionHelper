#pragma once

#include "golarion/data/movement_save_data.hpp"

#include <vector>

namespace golarion
{
    struct MovementGroupSaveData
    {
        std::vector<MovementGrantSaveData> grants;
        std::vector<MovementAdjustmentSaveData> adjustments;
    };
}
