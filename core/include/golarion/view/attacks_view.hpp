#pragma once

#include "golarion/character/attack_routine.hpp"
#include "golarion/view/attack_routines_view.hpp"
#include "golarion/view/strikes_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct CalculatedDamageAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int abilityContribution;
        int bonus;
    };

    struct CalculatedDamageComponentView
    {
        DamageComponentView component;
        int occurrences;
    };

    struct CalculatedDamageView
    {
        std::vector<CalculatedDamageAbilityOptionView> abilityOptions;
        std::vector<CalculatedDamageComponentView> components;
    };

    struct CalculatedAttackView
    {
        std::string slotId;
        std::string slotName;
        std::string strikeGrantId;
        std::string strikeName;
        std::optional<std::string> progressionGrantId;
        std::optional<std::string> progressionGrantSource;
        int progressionAttackBonusAdjustment;
        int routineAttackBonusAdjustment;
        int totalAttackBonusAdjustment;
        StrikeView strike;
        CalculatedDamageView normalDamage;
        CalculatedDamageView criticalDamage;
    };

    struct AttackSlotView
    {
        std::string id;
        std::string name;
        StrikeSelectorView selector;
        RoutineSlotSelectionMode selectionMode;
        StrikeUsage strikeUsage;
        StrikeUsage effectiveStrikeUsage;
        std::string baseAttackBonusAdjustmentExpression;
        int baseAttackBonusAdjustment;
        int effectiveAttackBonusAdjustment;
        std::vector<RoutineAttackProgressionView> progressions;
        std::vector<RoutineAttackBonusAdjustmentView> appliedAttackBonusAdjustments;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride;
        std::optional<HandUsage> handUsage;
        std::optional<AttackHandRole> handRole;
        bool assignmentRequired;
        std::optional<std::string> assignedGrantId;
        std::vector<std::string> effectiveGrantIds;
        std::vector<RoutineStrikeCandidateView> candidates;
        bool complete;
        bool usable;
        std::vector<std::string> failureReasons;
    };

    struct AttackView
    {
        std::string id;
        std::string name;
        std::string routineId;
        std::optional<std::string> routineName;
        std::optional<std::string> actionId;
        std::optional<std::string> actionName;
        bool complete;
        bool usable;
        std::vector<std::string> failureReasons;
        std::vector<AttackSlotView> slots;
        std::vector<CalculatedAttackView> calculatedAttacks;
    };

    struct AttacksView
    {
        std::vector<AttackView> attacks;
    };
}
