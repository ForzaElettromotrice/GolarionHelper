#pragma once

#include "golarion/util/game_duration.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct TemporaryHitPointPoolSaveData
    {
        std::string id;
        int remaining;
        std::optional<GameDuration> duration;
    };

    struct TemporaryHitPointsSaveData
    {
        std::vector<TemporaryHitPointPoolSaveData> pools;
    };
}
