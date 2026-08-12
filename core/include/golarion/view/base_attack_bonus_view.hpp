#pragma once

#include "golarion/view/contribution_set_view.hpp"

namespace golarion
{
    struct BaseAttackBonusView
    {
        int total;
        ContributionSetView contributions;
    };
}
