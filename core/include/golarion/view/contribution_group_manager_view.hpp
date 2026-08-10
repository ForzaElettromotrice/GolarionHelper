#pragma once

#include "golarion/view/contribution_group_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ContributionGroupManagerView
    {
        struct GroupView
        {
            std::string id;
            bool enabled;
            ContributionGroupView group;
        };

        std::vector<GroupView> groups;
    };
}
