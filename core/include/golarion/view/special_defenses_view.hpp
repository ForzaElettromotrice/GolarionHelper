#pragma once

#include "golarion/character/damage.hpp"
#include "golarion/character/damage_reduction.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct EnergyResistanceAdjustmentApplicationView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedIncrease;
        std::string stackingGroup;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct EnergyResistanceGrantView
    {
        std::string id;
        std::string source;
        DamageType energy;
        std::string expression;
        int baseValue;
        int effectiveValue;
        std::vector<std::string> tags;
        std::vector<EnergyResistanceAdjustmentApplicationView> adjustments;
        bool determinesEffectiveValue;
        std::optional<std::string> notAppliedReason;
    };

    struct EnergyResistanceView
    {
        DamageType energy;
        int effectiveValue;
        std::vector<EnergyResistanceGrantView> grants;
    };

    struct DamageReductionAdjustmentApplicationView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedDelta;
        std::string stackingGroup;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct DamageReductionGrantView
    {
        std::string id;
        std::string source;
        std::string expression;
        int baseValue;
        int effectiveValue;
        DamageReductionBypass bypass;
        std::optional<std::string> applicability;
        std::vector<std::string> tags;
        std::vector<std::string> stacksWithTags;
        std::vector<std::string> stackableWithGrantIds;
        std::vector<DamageReductionAdjustmentApplicationView> adjustments;
    };

    struct DamageReductionStackingAuthorizationView
    {
        std::string declaringGrantId;
        std::string compatibleGrantId;
        std::vector<std::string> matchedTags;
    };

    struct DamageReductionCombinationView
    {
        std::vector<std::string> grantIds;
        int maximumValue;
        std::optional<DamageReductionBypass> flattenedBypass;
        std::optional<std::string> flattenedApplicability;
        std::vector<DamageReductionStackingAuthorizationView> authorizations;
    };

    struct ImmunityGrantView
    {
        std::string id;
        std::string source;
        std::optional<std::string> applicability;
    };

    struct ImmunityView
    {
        std::string targetId;
        std::string name;
        std::vector<ImmunityGrantView> grants;
    };

    struct SpellResistanceAdjustmentApplicationView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedDelta;
        std::string stackingGroup;
        std::optional<std::string> applicability;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct SpellResistanceGrantView
    {
        std::string id;
        std::string source;
        std::string expression;
        int baseValue;
        int effectiveValue;
        std::optional<std::string> applicability;
        bool canBeLowered;
        std::vector<std::string> tags;
        std::vector<SpellResistanceAdjustmentApplicationView> adjustments;
        bool determinesEffectiveValue;
        std::optional<std::string> notAppliedReason;
    };

    struct SpellResistanceContextView
    {
        std::optional<std::string> applicability;
        int effectiveValue;
        std::vector<SpellResistanceGrantView> grants;
    };

    struct FastHealingGrantView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedValue;
        std::optional<std::string> applicability;
        std::string stackingGroup;
        std::vector<std::string> tags;
        bool contributesToEffectiveValue;
        std::optional<std::string> notAppliedReason;
    };

    struct FastHealingContextView
    {
        std::optional<std::string> applicability;
        int effectiveValue;
        std::vector<FastHealingGrantView> grants;
    };

    struct RegenerationGrantView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedValue;
        std::optional<std::string> interruption;
        std::optional<std::string> applicability;
        std::vector<std::string> tags;
    };

    struct SpecialDefensesView
    {
        std::vector<EnergyResistanceView> energyResistances;
        std::vector<DamageReductionGrantView> damageReductions;
        std::vector<DamageReductionCombinationView> damageReductionCombinations;
        std::vector<ImmunityView> immunities;
        std::vector<SpellResistanceContextView> spellResistances;
        std::vector<FastHealingContextView> fastHealing;
        std::vector<RegenerationGrantView> regeneration;
    };
}
