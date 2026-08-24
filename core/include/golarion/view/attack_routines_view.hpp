#pragma once

#include "golarion/character/attack_routine.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct StrikeSelectorView
    {
        std::vector<AttackMode> allowedModes;
        std::vector<AttackTag> requiredTags;
        std::vector<AttackTag> forbiddenTags;
        std::vector<std::string> allowedGrantIds;
    };

    struct RoutineStrikeCandidateView
    {
        std::string grantId;
        std::string name;
        std::vector<std::string> usageChannels;
        bool accepted;
        std::vector<std::string> rejectionReasons;
    };

    struct RoutineAttackProgressionView
    {
        RoutineAttackProgressionType type;
        std::optional<std::string> countExpression;
        std::string attackBonusAdjustmentExpression;
        std::vector<int> attackBonusAdjustments;
        std::optional<std::string> grantId;
        std::optional<std::string> grantSource;
    };

    struct RoutineAssignmentRequirementView
    {
        std::string slotId;
        std::optional<std::string> strikeGrantId;
        std::optional<AttackTag> requiredTag;
        std::optional<WeaponWeightPurpose> weaponWeightPurpose;
        std::optional<WeaponWeight> weaponWeight;
    };

    struct RoutineAttackBonusAdjustmentView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedValue;
        std::vector<RoutineAssignmentRequirementView> requirements;
    };

    struct RoutineSlotView
    {
        std::string id;
        std::string name;
        StrikeSelectorView selector;
        RoutineSlotSelectionMode selectionMode;
        StrikeUsage strikeUsage;
        std::string baseAttackBonusAdjustmentExpression;
        int baseAttackBonusAdjustment;
        std::vector<RoutineAttackProgressionView> progressions;
        std::vector<RoutineAttackBonusAdjustmentView> attackBonusAdjustments;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride;
        std::vector<RoutineStrikeCandidateView> candidates;
    };

    struct AttackRoutineView
    {
        std::string id;
        std::string source;
        std::string name;
        std::vector<RoutineSlotView> slots;
    };

    struct AttackRoutinesView
    {
        std::vector<AttackRoutineView> routines;
    };
}
