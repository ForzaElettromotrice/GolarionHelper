#pragma once

#include "golarion/view/movement_group_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct MovementGroupManagerView
    {
        struct GroupView
        {
            std::string id;
            bool enabled;
            MovementGroupView group;
        };

        std::vector<GroupView> groups;
    };
}
