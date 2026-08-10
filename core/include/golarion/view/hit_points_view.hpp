#pragma once

#include "golarion/view/contribution_set_view.hpp"
#include "golarion/view/temporary_hit_points_view.hpp"

namespace golarion
{
    struct HitPointsView
    {
        int baseMax;
        int max;
        int current;
        TemporaryHitPointsView temporary;
        int nonLethal;
        ContributionSetView maxContributions;
    };
}
