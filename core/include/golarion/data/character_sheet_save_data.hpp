#pragma once

#include "golarion/data/ability_save_data.hpp"
#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/data/skills_save_data.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetSaveData
    {
        int formatVersion;
        std::vector<AbilitySaveData> abilities;
        HitPointsSaveData hitPoints;
        SkillsSaveData skills;
    };
}
