#pragma once

#include "golarion/character/strike.hpp"
#include "golarion/view/attack_distance_view.hpp"
#include "golarion/view/damage_view.hpp"
#include "golarion/view/modifier_set_view.hpp"
#include "golarion/view/requirement_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct CriticalAdjustmentView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        CriticalAdjustmentType type;
        std::string expression;
        int resolvedValue;
        std::optional<int> maximumMultiplier;
        std::optional<std::string> condition;
    };

    struct CriticalProfileView
    {
        int threatMinimum;
        int multiplier;
    };

    struct ConditionalCriticalProfileView
    {
        std::string condition;
        CriticalProfileView profile;
    };

    struct AttackAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int attackBonus;
        int criticalConfirmationBonus;
    };

    struct DamageAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int abilityContribution;
        int damageBonus;
        int criticalDamageBonus;
    };

    struct AttackRequirementView
    {
        std::optional<std::string> id;
        std::string source;
        std::optional<std::string> targetResourceName;
        RequirementView requirement;
        std::optional<std::string> condition;
    };

    struct ConditionalAttackUsabilityView
    {
        std::string condition;
        bool usable;
        std::vector<std::string> failureReasons;
    };

    struct AttackDefenseOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        ArmorClassType defenseType;
        std::optional<std::string> condition;
    };

    struct StrikeWeaponWeightAdjustmentView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        WeaponWeightPurpose purpose;
        WeaponWeight weaponWeight;
    };

    struct StrikeUsageAdjustmentView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        StrikeUsage usage;
        std::optional<std::string> attackPenaltyExpression;
        std::optional<int> resolvedAttackPenalty;
        std::optional<DamageAbilityRule> damageAbilityRule;
    };

    struct StrikeView
    {
        std::string grantId;
        std::string source;
        std::string name;
        AttackMode mode;
        std::vector<AttackTag> tags;
        StrikeUsage usage;
        std::optional<NaturalAttackClassification> naturalAttackClassification;
        std::vector<std::string> usageChannels;
        std::optional<WeaponWeight> baseWeaponWeight;
        std::optional<WeaponWeight> effectiveWeaponWeight;
        std::optional<WeaponWeight> twoWeaponFightingWeaponWeight;
        std::vector<StrikeWeaponWeightAdjustmentView> weaponWeightAdjustments;
        int usageAttackPenalty;
        std::vector<StrikeUsageAdjustmentView> usageAdjustments;
        std::vector<AttackDefenseOptionView> defenseOptions;
        std::vector<AttackAbilityOptionView> attackAbilityOptions;
        ModifierSetView attackModifiers;
        ModifierSetView criticalConfirmationModifiers;
        CriticalProfileView criticalProfile;
        std::vector<ConditionalCriticalProfileView> conditionalCriticalProfiles;
        std::vector<CriticalAdjustmentView> criticalAdjustments;
        AbilityType damageAbility;
        DamageAbilityRule damageAbilityRule;
        std::vector<DamageAbilityOptionView> damageAbilityOptions;
        ModifierSetView damageModifiers;
        std::vector<DamageComponentView> damageComponents;
        std::optional<AttackReachView> reach;
        std::optional<AttackRangeView> range;
        bool usable;
        std::vector<std::string> failureReasons;
        std::vector<ConditionalAttackUsabilityView> conditionalUsability;
        std::vector<AttackRequirementView> requirements;
    };

    struct StrikesView
    {
        std::vector<std::string> activeConditions;
        std::vector<StrikeView> strikes;
    };
}
