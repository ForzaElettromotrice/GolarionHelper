#pragma once

#include "golarion/data/skill_save_data.hpp"

#include <vector>

namespace golarion
{
    struct SkillsSaveData
    {
        std::vector<SkillSaveData> skills;
    };
}
