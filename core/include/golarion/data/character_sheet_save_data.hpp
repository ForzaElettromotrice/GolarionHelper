#pragma once

#include "golarion/data/ability_save_data.hpp"
#include "golarion/data/attacks_data.hpp"
#include "golarion/data/character_identity_save_data.hpp"
#include "golarion/data/condition_manager_save_data.hpp"
#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/data/inventory_save_data.hpp"
#include "golarion/data/race_save_data.hpp"
#include "golarion/data/skills_save_data.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetSaveData
    {
        int formatVersion;
        CharacterIdentitySaveData identity;
        RaceSaveData race;
        std::vector<AbilitySaveData> abilities;
        HitPointsSaveData hitPoints;
        SkillsSaveData skills;
        AttacksData attacks;
        ConditionManagerSaveData conditions;
        InventorySaveData inventory;
    };
}
