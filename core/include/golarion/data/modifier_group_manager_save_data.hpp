#pragma once

#include "golarion/data/modifier_group_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ModifierGroupManagerSaveData
    {
        struct GroupSaveData
        {
            std::string id;
            bool enabled;
            ModifierGroupSaveData group;
        };

        std::vector<GroupSaveData> groups;
    };
}
