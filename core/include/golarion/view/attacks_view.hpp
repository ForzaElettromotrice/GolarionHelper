#pragma once

#include "golarion/character/attack_routine.hpp"
#include "golarion/view/attack_routines_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct AttackSlotView
    {
        std::string id;
        std::string name;
        StrikeSelectorView selector;
        RoutineSlotSelectionMode selectionMode;
        StrikeUsage strikeUsage;
        std::string baseAttackBonusAdjustmentExpression;
        int baseAttackBonusAdjustment;
        int effectiveAttackBonusAdjustment;
        std::vector<RoutineAttackProgressionView> progressions;
        std::vector<RoutineAttackBonusAdjustmentView> appliedAttackBonusAdjustments;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride;
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
        bool complete;
        bool usable;
        std::vector<std::string> failureReasons;
        std::vector<AttackSlotView> slots;
    };

    struct AttacksView
    {
        std::vector<AttackView> attacks;
    };
}
