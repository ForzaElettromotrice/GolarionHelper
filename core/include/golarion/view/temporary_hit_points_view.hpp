#pragma once

#include "golarion/util/game_duration.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct TemporaryHitPointPoolView
    {
        std::string id;
        int remaining;
        std::optional<GameDuration> duration;
    };

    struct TemporaryHitPointsView
    {
        int total;
        std::vector<TemporaryHitPointPoolView> pools;
    };
}
