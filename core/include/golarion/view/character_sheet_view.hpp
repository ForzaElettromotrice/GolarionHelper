#pragma once

#include "golarion/view/ability_view.hpp"
#include "golarion/view/contribution_group_manager_view.hpp"
#include "golarion/view/hit_points_view.hpp"
#include "golarion/view/initiative_view.hpp"
#include "golarion/view/modifier_group_manager_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/movement_group_manager_view.hpp"
#include "golarion/view/resource_manager_view.hpp"
#include "golarion/view/saving_throws_view.hpp"
#include "golarion/view/skills_view.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetView
    {
        std::vector<AbilityView> abilities;
        HitPointsView hitPoints;
        InitiativeView initiative;
        SavingThrowsView savingThrows;
        SkillsView skills;
        MovementView movement;
        MovementGroupManagerView movementGroups;
        ResourceManagerView resources;
        ModifierGroupManagerView modifierGroups;
        ContributionGroupManagerView contributionGroups;
    };
}
