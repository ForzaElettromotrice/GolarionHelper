#pragma once

#include "golarion/data/contribution_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ContributionGroupSaveData
    {
        struct TargetedContributionSaveData
        {
            std::string resourceName;
            ContributionSaveData contribution;
        };

        std::vector<TargetedContributionSaveData> contributions;
    };
}
