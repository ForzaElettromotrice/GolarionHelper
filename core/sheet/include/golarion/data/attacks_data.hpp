#pragma once

#include "golarion/data/attack_data.hpp"

#include <vector>

namespace golarion
{
    struct AttacksData
    {
        std::vector<AttackData> attacks;
    };
}
