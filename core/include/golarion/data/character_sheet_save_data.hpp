#pragma once

#include "golarion/data/ability_save_data.hpp"
#include "golarion/data/contribution_group_manager_save_data.hpp"
#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/data/initiative_save_data.hpp"
#include "golarion/data/modifier_group_manager_save_data.hpp"
#include "golarion/data/movement_group_manager_save_data.hpp"
#include "golarion/data/saving_throws_save_data.hpp"
#include "golarion/data/skills_save_data.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetSaveData
    {
        int formatVersion;
        std::vector<AbilitySaveData> abilities;
        HitPointsSaveData hitPoints;
        InitiativeSaveData initiative;
        SavingThrowsSaveData savingThrows;
        SkillsSaveData skills;
        MovementGroupManagerSaveData movementGroups;
        ModifierGroupManagerSaveData modifierGroups;
        ContributionGroupManagerSaveData contributionGroups;
    };
}
