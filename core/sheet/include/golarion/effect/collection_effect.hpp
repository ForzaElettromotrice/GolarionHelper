#pragma once

#include "golarion/character/armor_class.hpp"
#include "golarion/character/action.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/saving_throw.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/character/special_defenses.hpp"
#include "golarion/character/temporary_hit_points.hpp"
#include <variant>

namespace golarion
{
    struct AbilityScoreReplacementEffectEntry : AbilityReplacementDefinition
    {
        AbilityType ability;
    };
    struct RoutineSlotEffectEntry
    {
        std::string id;
        std::string name;
        StrikeSelector selector;
        RoutineSlotSelectionMode selectionMode;
        StrikeUsage strikeUsage;
        std::vector<RoutineAttackProgressionDefinition> progressions;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride;
        std::string baseAttackBonusAdjustmentExpression = "0";
        std::optional<HandUsage> handUsage;
        std::optional<AttackHandRole> handRole;
        bool assignmentRequired = true;
    };
    struct AttackRoutineEffectEntry
    {
        std::string id;
        std::string source;
        std::string name;
        std::string actionId;
        std::vector<RoutineSlotEffectEntry> slots;
    };
    struct RoutineProgressionEffectEntry
    {
        std::string id;
        std::string source;
        std::string targetRoutineId;
        std::string targetSlotId;
        RoutineAttackProgressionDefinition progression;
    };
    // Definitions reuse endpoint schemas. Source and entry IDs are supplied by the owner.
    using CollectionEffectEntry = std::variant<
        EnergyResistanceDefinition, EnergyResistanceAdjustmentDefinition, EnergyVulnerabilityDefinition,
        DamageReductionDefinition, DamageReductionAdjustmentDefinition, ImmunityDefinition,
        SpellResistanceDefinition, SpellResistanceAdjustmentDefinition, FastHealingDefinition, RegenerationDefinition,
        SizeReplacementDefinition, SizeAdjustmentDefinition, CarryingCapacityMultiplierDefinition,
        MovementAdjustmentDefinition, RunAdjustmentDefinition, MovementBoundedBonus,
        ArmorClassAbilityReplacementDefinition, ArmorClassAbilitySuppressionDefinition, ArmorClassDexterityRetention,
        MaximumDexterityLimitDefinition, ArmorCheckPenaltyDefinition, SkillAbilityReplacementDefinition,
        SkillClassSkillGrantDefinition, InitiativeAbilityReplacementDefinition, SavingThrowAbilityReplacementDefinition,
        TemporaryHitPointGrant, ConditionEntryDefinition,
        AbilityScoreReplacementEffectEntry, CombatManeuverAbilityReplacementDefinition, CombatManeuverDefenseDexteritySuppressionDefinition,
        AttackRoutineEffectEntry, RoutineProgressionEffectEntry, RoutineAttackBonusAdjustmentDefinition,
        ActionCategoryDefinition, ActionDefinition, ActionInhibitionDefinition, ActionNoteDefinition, ActionCostReplacementDefinition,
        StrikeGrantDefinition, DamageComponentGrantDefinition, CriticalAdjustmentDefinition, AttackAbilityReplacementDefinition,
        DamageAbilityReplacementDefinition, DamageDiceAdjustmentDefinition, AttackDistanceAdjustmentDefinition,
        AttackRequirementDefinition, AttackDefenseReplacementDefinition, StrikeWeaponWeightAdjustmentDefinition, StrikeUsageAdjustmentDefinition>;

    struct CollectionEffectDefinition
    {
        std::string id;
        CollectionEffectEntry entry;
    };
}
