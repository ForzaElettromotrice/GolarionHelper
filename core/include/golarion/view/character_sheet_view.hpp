#pragma once

#include "golarion/view/ability_view.hpp"
#include "golarion/view/armor_class_view.hpp"
#include "golarion/view/attacks_view.hpp"
#include "golarion/view/base_attack_bonus_view.hpp"
#include "golarion/view/combat_maneuvers_view.hpp"
#include "golarion/view/hit_points_view.hpp"
#include "golarion/view/initiative_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/saving_throws_view.hpp"
#include "golarion/view/skills_view.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetView
    {
        std::vector<AbilityView> abilities;
        BaseAttackBonusView baseAttackBonus;
        AttacksView attacks;
        CombatManeuversView combatManeuvers;
        HitPointsView hitPoints;
        InitiativeView initiative;
        ArmorClassView armorClass;
        SavingThrowsView savingThrows;
        SkillsView skills;
        MovementView movement;
    };
}
