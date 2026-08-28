#pragma once

#include "golarion/view/actions_view.hpp"
#include "golarion/view/ability_view.hpp"
#include "golarion/view/armor_class_view.hpp"
#include "golarion/view/attack_routines_view.hpp"
#include "golarion/view/attacks_view.hpp"
#include "golarion/view/strikes_view.hpp"
#include "golarion/view/base_attack_bonus_view.hpp"
#include "golarion/view/carrying_capacity_view.hpp"
#include "golarion/view/character_identity_view.hpp"
#include "golarion/view/combat_maneuvers_view.hpp"
#include "golarion/view/condition_manager_view.hpp"
#include "golarion/view/encumbrance_view.hpp"
#include "golarion/view/hit_points_view.hpp"
#include "golarion/view/initiative_view.hpp"
#include "golarion/view/inventory_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/reminders_view.hpp"
#include "golarion/view/saving_throws_view.hpp"
#include "golarion/view/size_view.hpp"
#include "golarion/view/skills_view.hpp"
#include "golarion/view/special_defenses_view.hpp"

#include <vector>

namespace golarion
{
    struct CharacterSheetView
    {
        CharacterIdentityView identity;
        ActionsView actions;
        RemindersView reminders;
        std::vector<AbilityView> abilities;
        BaseAttackBonusView baseAttackBonus;
        StrikesView strikes;
        AttackRoutinesView attackRoutines;
        AttacksView attacks;
        CombatManeuversView combatManeuvers;
        HitPointsView hitPoints;
        InitiativeView initiative;
        ArmorClassView armorClass;
        SavingThrowsView savingThrows;
        SpecialDefensesView specialDefenses;
        SkillsView skills;
        MovementView movement;
        CarryingCapacityView carryingCapacity;
        EncumbranceView encumbrance;
        InventoryView inventory;
        SizeView size;
        ConditionManagerView conditions;
    };
}
