#pragma once

#include "golarion/view/contribution_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ContributionGroupView
    {
        struct TargetedContributionView
        {
            std::string resourceName;
            ContributionView contribution;
        };

        std::vector<TargetedContributionView> contributions;
    };
}
