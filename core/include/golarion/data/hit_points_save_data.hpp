#pragma once

#include "golarion/data/temporary_hit_points_save_data.hpp"

namespace golarion
{
    struct HitPointsSaveData
    {
        int baseMax;
        int damageTaken;
        TemporaryHitPointsSaveData temporary;
        int nonLethal;
        bool dead = false;
    };
}
