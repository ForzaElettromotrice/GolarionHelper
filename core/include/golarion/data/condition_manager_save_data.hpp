#pragma once

#include "golarion/data/condition_entry_save_data.hpp"

#include <vector>

namespace golarion
{
    struct ConditionManagerSaveData
    {
        std::vector<ConditionEntrySaveData> manualEntries;
    };
}
