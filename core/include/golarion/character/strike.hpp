#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/attack_distance.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/damage.hpp"
#include "golarion/resource/requirement.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view StrikeGrantsResource = "strike.grants";
    inline constexpr std::string_view DamageComponentGrantsResource = "damage.componentGrants";
    inline constexpr std::string_view CriticalAdjustmentsResource = "attack.criticalAdjustments";
    inline constexpr std::string_view AttackAbilityReplacementsResource = "attack.abilityReplacements";
    inline constexpr std::string_view DamageAbilityReplacementsResource = "damage.abilityReplacements";
    inline constexpr std::string_view DamageDiceAdjustmentsResource = "damage.diceAdjustments";
    inline constexpr std::string_view AttackRequirementsResource = "attack.requirements";
    inline constexpr std::string_view AttackDefenseReplacementsResource = "attack.defenseReplacements";
    inline constexpr std::string_view StrikeWeaponWeightAdjustmentsResource = "strike.weaponWeightAdjustments";
    inline constexpr std::string_view StrikeUsageAdjustmentsResource = "strike.usageAdjustments";
    inline constexpr std::string_view WeaponDamageRollResource = "damage.weaponRoll";

    class ResourceManager;
    struct StrikesView;

    enum class AttackMode
    {
        Melee,
        Ranged
    };

    enum class AttackTag
    {
        Weapon,
        Thrown,
        Projectile,
        Natural,
        Unarmed
    };

    enum class NaturalAttackClassification
    {
        Primary,
        Secondary
    };

    enum class StrikeUsage
    {
        Default,
        NaturalSecondary,
        SingleNatural,
        NaturalSecondaryWhenCombined
    };

    enum class WeaponWeight
    {
        Light,
        OneHanded,
        TwoHanded
    };

    enum class WeaponWeightPurpose
    {
        General,
        TwoWeaponFighting
    };

    struct StrikeUsageOverride
    {
        std::string grantId;
        StrikeUsage usage;
    };

    struct StrikeCalculationContext
    {
        std::vector<std::string> activeConditions;
        std::vector<StrikeUsageOverride> usageOverrides;
    };

    enum class DamageAbilityRule
    {
        None,
        Full,
        HalfPositiveFullPenalty,
        OneAndHalfPositiveFullPenalty,
        PenaltyOnly
    };

    enum class CriticalAdjustmentType
    {
        ThreatRangeMultiplier,
        ThreatMinimum,
        MultiplierIncrease,
        MultiplierSet
    };

    std::string_view displayName(AttackMode mode);
    std::string_view displayName(AttackTag tag);
    std::string_view displayName(NaturalAttackClassification classification);
    std::string_view displayName(StrikeUsage usage);
    std::string_view displayName(WeaponWeight weight);
    std::string_view displayName(WeaponWeightPurpose purpose);
    std::string_view displayName(DamageAbilityRule rule);
    std::string_view displayName(CriticalAdjustmentType type);
    int damageAbilityContribution(int abilityModifier, DamageAbilityRule rule);
    AbilityType defaultAttackAbility(AttackMode mode);
    std::string attackResourceName(AttackMode mode);
    std::string attackResourceName(AttackTag tag);
    std::string attackResourceName(std::string_view strikeId);
    std::string damageResourceName(AttackMode mode);
    std::string damageResourceName(AttackTag tag);
    std::string damageResourceName(std::string_view strikeId);
    std::string criticalConfirmationResourceName(AttackMode mode);
    std::string criticalConfirmationResourceName(AttackTag tag);
    std::string criticalConfirmationResourceName(std::string_view strikeId);

    struct StrikeGrantDefinition
    {
        std::string id;
        std::string source;
        std::string name;
        AttackMode mode;
        std::vector<AttackTag> tags;
        std::optional<NaturalAttackClassification> naturalAttackClassification;
        std::vector<std::string> usageChannels;
        std::vector<DamageComponent> damageComponents;
        AbilityType damageAbility;
        DamageAbilityRule damageAbilityRule;
        int criticalThreatMinimum;
        int criticalMultiplier;
        ArmorClassType defenseType = ArmorClassType::Normal;
        std::optional<AttackReachDefinition> reach;
        std::optional<AttackRangeDefinition> range;
        std::vector<Requirement> requirements;
        std::optional<WeaponWeight> weaponWeight;
    };

    class StrikeGrant final
    {
    public:
        explicit StrikeGrant(StrikeGrantDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string name_;
        AttackMode mode_;
        std::vector<AttackTag> tags_;
        std::optional<NaturalAttackClassification> naturalAttackClassification_;
        std::vector<std::string> usageChannels_;
        std::vector<DamageComponent> damageComponents_;
        AbilityType damageAbility_;
        DamageAbilityRule damageAbilityRule_;
        int criticalThreatMinimum_;
        int criticalMultiplier_;
        ArmorClassType defenseType_;
        std::optional<AttackReachDefinition> reach_;
        std::optional<AttackRangeDefinition> range_;
        std::vector<Requirement> requirements_;
        std::optional<WeaponWeight> weaponWeight_;
    };

    struct StrikeWeaponWeightAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        WeaponWeightPurpose purpose;
        WeaponWeight weaponWeight;
    };

    class StrikeWeaponWeightAdjustment final
    {
    public:
        explicit StrikeWeaponWeightAdjustment(StrikeWeaponWeightAdjustmentDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        WeaponWeightPurpose purpose_;
        WeaponWeight weaponWeight_;
    };

    struct StrikeUsageAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        StrikeUsage usage;
        std::optional<std::string> attackPenaltyExpression;
        std::optional<DamageAbilityRule> damageAbilityRule;
    };

    class StrikeUsageAdjustment final
    {
    public:
        explicit StrikeUsageAdjustment(StrikeUsageAdjustmentDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        StrikeUsage usage_;
        std::optional<std::string> attackPenaltyExpression_;
        std::optional<DamageAbilityRule> damageAbilityRule_;
    };

    struct AttackRequirementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        Requirement requirement;
        std::optional<std::string> condition;
    };

    class AttackRequirement final
    {
    public:
        explicit AttackRequirement(AttackRequirementDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        Requirement requirement_;
        std::optional<std::string> condition_;
    };

    struct AttackDefenseReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        ArmorClassType defenseType;
        std::optional<std::string> condition;
    };

    class AttackDefenseReplacement final
    {
    public:
        explicit AttackDefenseReplacement(AttackDefenseReplacementDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        ArmorClassType defenseType_;
        std::optional<std::string> condition_;
    };

    struct DamageComponentGrantDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        DamageComponent component;
    };

    class DamageComponentGrant final
    {
    public:
        explicit DamageComponentGrant(DamageComponentGrantDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        DamageComponent component_;
    };

    struct CriticalAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        CriticalAdjustmentType type;
        std::string expression;
        std::optional<int> maximumMultiplier;
        std::optional<std::string> condition;
    };

    class CriticalAdjustment final
    {
    public:
        explicit CriticalAdjustment(CriticalAdjustmentDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        CriticalAdjustmentType type_;
        std::string expression_;
        std::optional<int> maximumMultiplier_;
        std::optional<std::string> condition_;
    };

    struct AttackAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AbilityType abilityType;
    };

    class AttackAbilityReplacement final
    {
    public:
        explicit AttackAbilityReplacement(AttackAbilityReplacementDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        AbilityType abilityType_;
    };

    struct DamageAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AbilityType abilityType;
    };

    class DamageAbilityReplacement final
    {
    public:
        explicit DamageAbilityReplacement(DamageAbilityReplacementDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        AbilityType abilityType_;
    };

    struct DamageDiceAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        DamageComponentRole targetRole;
        DamageComponentOriginFilter targetOrigin;
        std::optional<std::string> targetComponentGrantId;
        std::optional<std::string> targetComponentId;
        DamageDiceAdjustmentType type;
        std::optional<std::string> expression;
        std::optional<DamageDice> setDice;
        std::optional<std::string> condition;
    };

    class DamageDiceAdjustment final
    {
    public:
        explicit DamageDiceAdjustment(DamageDiceAdjustmentDefinition definition);

    private:
        friend class Strikes;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        DamageComponentRole targetRole_;
        DamageComponentOriginFilter targetOrigin_;
        std::optional<std::string> targetComponentGrantId_;
        std::optional<std::string> targetComponentId_;
        DamageDiceAdjustmentType type_;
        std::optional<std::string> expression_;
        std::optional<DamageDice> setDice_;
        std::optional<std::string> condition_;
    };

    class Strikes final
    {
    public:
        explicit Strikes(ResourceManager &resourceManager);

        StrikesView toView();
        StrikesView toView(const StrikeCalculationContext &context);

    private:
        void addGrant(StrikeGrant grant);
        void removeGrant(std::string_view grantId);
        void addDamageComponentGrant(DamageComponentGrant grant);
        void removeDamageComponentGrant(std::string_view grantId);
        void addCriticalAdjustment(CriticalAdjustment adjustment);
        void removeCriticalAdjustment(std::string_view adjustmentId);
        void addAttackAbilityReplacement(AttackAbilityReplacement replacement);
        void removeAttackAbilityReplacement(std::string_view replacementId);
        void addDamageAbilityReplacement(DamageAbilityReplacement replacement);
        void removeDamageAbilityReplacement(std::string_view replacementId);
        void addDamageDiceAdjustment(DamageDiceAdjustment adjustment);
        void removeDamageDiceAdjustment(std::string_view adjustmentId);
        void addDistanceAdjustment(AttackDistanceAdjustment adjustment);
        void removeDistanceAdjustment(std::string_view adjustmentId);
        void addRequirement(AttackRequirement requirement);
        void removeRequirement(std::string_view requirementId);
        void addDefenseReplacement(AttackDefenseReplacement replacement);
        void removeDefenseReplacement(std::string_view replacementId);
        void addWeaponWeightAdjustment(StrikeWeaponWeightAdjustment adjustment);
        void removeWeaponWeightAdjustment(std::string_view adjustmentId);
        void addUsageAdjustment(StrikeUsageAdjustment adjustment);
        void removeUsageAdjustment(std::string_view adjustmentId);
        std::vector<std::string> attackParentResources(const StrikeGrant &grant) const;
        std::vector<std::string> damageParentResources(const StrikeGrant &grant) const;
        std::vector<std::string> criticalConfirmationParentResources(const StrikeGrant &grant) const;
        std::vector<std::string> distanceParentResources(AttackDistanceProperty property, const StrikeGrant &grant) const;

        ResourceManager &resourceManager_;
        std::vector<StrikeGrant> grants_;
        std::map<std::string, DamageComponentGrant> damageComponentGrants_;
        std::map<std::string, CriticalAdjustment> criticalAdjustments_;
        std::map<std::string, AttackAbilityReplacement> attackAbilityReplacements_;
        std::map<std::string, DamageAbilityReplacement> damageAbilityReplacements_;
        std::map<std::string, DamageDiceAdjustment> damageDiceAdjustments_;
        std::map<std::string, AttackDistanceAdjustment> distanceAdjustments_;
        std::map<std::string, AttackRequirement> requirements_;
        std::map<std::string, AttackDefenseReplacement> defenseReplacements_;
        std::map<std::string, StrikeWeaponWeightAdjustment> weaponWeightAdjustments_;
        std::map<std::string, StrikeUsageAdjustment> usageAdjustments_;
    };
}
