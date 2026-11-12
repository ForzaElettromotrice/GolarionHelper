#pragma once
#include "golarion/character/feat_definition.hpp"
#include "golarion/character/spell_definition.hpp"
#include "golarion/equipment/item.hpp"
#include "golarion/view/enhancements_view.hpp"

#include "golarion/view/feats_view.hpp"
#include "golarion/view/classes_view.hpp"
#include "golarion/view/spellcastings_view.hpp"
#include "golarion/view/languages_view.hpp"
#include "golarion/character/granted_spell.hpp"

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
#include "golarion/view/race_view.hpp"
#include "golarion/view/reminders_view.hpp"
#include "golarion/view/saving_throws_view.hpp"
#include "golarion/view/size_view.hpp"
#include "golarion/view/skills_view.hpp"
#include "golarion/view/special_defenses_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct CharacterSheetView
    {
        CharacterIdentityView identity;
        RaceView race;
        FeatsView feats;
        LanguagesView languages;
        std::vector<GrantedSpellView> grantedSpells;
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
        ClassesView classes;
        bool legacyFirstLevelFeatSlot;
        SpellcastingsView spellcastings;
        EnhancementsView enhancements;
        std::vector<ItemDefinition> customItemDefinitions;
        std::vector<SpellDefinition> customSpellDefinitions;
        std::vector<FeatDefinition> customFeatDefinitions;
        std::string notes;
    };
}
