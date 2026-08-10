#pragma once

#include "golarion/view/modifier_group_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ModifierGroupManagerView
    {
        struct GroupView
        {
            std::string id;
            bool enabled;
            ModifierGroupView group;
        };

        std::vector<GroupView> groups;
    };
}
