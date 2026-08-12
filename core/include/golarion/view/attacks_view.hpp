#pragma once

#include "golarion/character/attack.hpp"
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
        std::vector<int> attackBonuses;
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

    struct AttackView
    {
        std::string id;
        std::string source;
        std::string name;
        AttackMode mode;
        std::vector<AttackTag> tags;
        std::vector<AttackDefenseOptionView> defenseOptions;
        std::vector<AttackAbilityOptionView> attackAbilityOptions;
        ModifierSetView attackModifiers;
        ModifierSetView criticalConfirmationModifiers;
        CriticalProfileView criticalProfile;
        std::vector<ConditionalCriticalProfileView> conditionalCriticalProfiles;
        std::vector<CriticalAdjustmentView> criticalAdjustments;
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

    struct AttacksView
    {
        std::vector<AttackView> attacks;
    };
}
