#pragma once

#include "golarion/data/contribution_group_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ContributionGroupManagerSaveData
    {
        struct GroupSaveData
        {
            std::string id;
            bool enabled;
            ContributionGroupSaveData group;
        };

        std::vector<GroupSaveData> groups;
    };
}
