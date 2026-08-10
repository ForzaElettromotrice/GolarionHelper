#pragma once

#include "golarion/data/movement_group_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct MovementGroupManagerSaveData
    {
        struct GroupSaveData
        {
            std::string id;
            bool enabled;
            MovementGroupSaveData group;
        };

        std::vector<GroupSaveData> groups;
    };
}
