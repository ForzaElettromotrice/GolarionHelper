#pragma once

#include "golarion/view/contribution_view.hpp"

#include <vector>

namespace golarion
{
    struct ContributionSetView
    {
        int total;
        std::vector<ContributionView> contributions;
    };
}
