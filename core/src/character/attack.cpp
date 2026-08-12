#include "golarion/character/attack.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/attacks_view.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace
{
    bool contains(const std::vector<golarion::AttackTag> &tags, golarion::AttackTag tag)
    {
        return std::ranges::find(tags, tag) != tags.end();
    }

    std::string_view resourceSegment(golarion::AttackMode mode)
    {
        switch (mode)
        {
            case golarion::AttackMode::Melee:
                return "melee";
            case golarion::AttackMode::Ranged:
                return "ranged";
        }

        throw std::invalid_argument("unknown attack mode");
    }

    std::string_view resourceSegment(golarion::AttackTag tag)
    {
        switch (tag)
        {
            case golarion::AttackTag::Weapon:
                return "weapon";
            case golarion::AttackTag::Thrown:
                return "thrown";
            case golarion::AttackTag::Projectile:
                return "projectile";
            case golarion::AttackTag::Natural:
                return "natural";
            case golarion::AttackTag::Unarmed:
                return "unarmed";
            case golarion::AttackTag::Primary:
                return "primary";
            case golarion::AttackTag::Secondary:
                return "secondary";
        }

        throw std::invalid_argument("unknown attack tag");
    }

    std::string categoryResourceName(std::string_view root, golarion::AttackMode mode)
    {
        return std::string(root) + "." + std::string(resourceSegment(mode));
    }

    std::string categoryResourceName(std::string_view root, golarion::AttackTag tag)
    {
        return std::string(root) + "." + std::string(resourceSegment(tag));
    }

    std::string grantResourceName(std::string_view root, std::string_view grantId)
    {
        return std::string(root) + ".grant." + golarion::normalize(grantId);
    }

    int checkedAttackValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("attack value is out of range");
        }
        return static_cast<int>(value);
    }

    int damageAbilityContribution(int abilityModifier, golarion::DamageAbilityRule rule)
    {
        switch (rule)
        {
            case golarion::DamageAbilityRule::None:
                return 0;
            case golarion::DamageAbilityRule::Full:
                return abilityModifier;
            case golarion::DamageAbilityRule::HalfPositiveFullPenalty:
                return abilityModifier > 0 ? abilityModifier / 2 : abilityModifier;
            case golarion::DamageAbilityRule::OneAndHalfPositiveFullPenalty:
                return abilityModifier > 0 ? checkedAttackValue(static_cast<long long>(abilityModifier) * 3 / 2) : abilityModifier;
            case golarion::DamageAbilityRule::PenaltyOnly:
                return std::min(abilityModifier, 0);
        }

        throw std::invalid_argument("unknown damage ability rule");
    }

    struct ResolvedCriticalAdjustment
    {
        golarion::CriticalAdjustmentType type;
        int value;
        std::optional<int> maximumMultiplier;
    };

    struct ResolvedDamageDiceAdjustment
    {
        golarion::DamageDiceAdjustmentType type;
        std::optional<int> value;
        std::optional<golarion::DamageDice> setDice;
    };

    struct ResolvedAttackDistanceAdjustment
    {
        golarion::AttackDistanceAdjustmentType type;
        int value;
    };

    int resolveAttackDistanceValue(int baseValue, int modifierTotal, const std::vector<ResolvedAttackDistanceAdjustment> &adjustments)
    {
        long long multiplier = 1;
        std::optional<int> minimum;
        std::optional<int> maximum;
        for (const ResolvedAttackDistanceAdjustment &adjustment : adjustments)
        {
            switch (adjustment.type)
            {
                case golarion::AttackDistanceAdjustmentType::Multiplier:
                    if (adjustment.value < 1)
                    {
                        throw std::invalid_argument("attack distance multiplier must be at least 1");
                    }
                    multiplier += adjustment.value - 1LL;
                    break;
                case golarion::AttackDistanceAdjustmentType::Minimum:
                    minimum = minimum.has_value() ? std::max(*minimum, adjustment.value) : adjustment.value;
                    break;
                case golarion::AttackDistanceAdjustmentType::Maximum:
                    maximum = maximum.has_value() ? std::min(*maximum, adjustment.value) : adjustment.value;
                    break;
            }
        }
        if (multiplier > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("attack distance multiplier is out of range");
        }

        long long value = (static_cast<long long>(baseValue) + modifierTotal) * multiplier;
        if (minimum.has_value())
        {
            value = std::max(value, static_cast<long long>(*minimum));
        }
        if (maximum.has_value())
        {
            value = std::min(value, static_cast<long long>(*maximum));
        }
        return checkedAttackValue(value);
    }

    constexpr std::array AttackDistanceProperties{
        golarion::AttackDistanceProperty::MinimumReach,
        golarion::AttackDistanceProperty::MaximumReach,
        golarion::AttackDistanceProperty::RangeIncrement,
        golarion::AttackDistanceProperty::MaximumRangeIncrements,
        golarion::AttackDistanceProperty::RangePenaltyPerAdditionalIncrement
    };

    golarion::DamageDice resolveDamageDice(const golarion::DamageDice &baseDice, const std::vector<ResolvedDamageDiceAdjustment> &adjustments)
    {
        std::optional<golarion::DamageDice> selectedSet;
        long long progressionSteps = 0;
        long long combinedMultiplier = 1;
        for (const ResolvedDamageDiceAdjustment &adjustment : adjustments)
        {
            switch (adjustment.type)
            {
                case golarion::DamageDiceAdjustmentType::ProgressionSteps:
                    progressionSteps += *adjustment.value;
                    break;
                case golarion::DamageDiceAdjustmentType::DiceCountMultiplier:
                    if (*adjustment.value < 1)
                    {
                        throw std::invalid_argument("damage dice count multiplier must be at least 1");
                    }
                    combinedMultiplier += *adjustment.value - 1LL;
                    break;
                case golarion::DamageDiceAdjustmentType::Set:
                    if (!selectedSet.has_value())
                    {
                        selectedSet = adjustment.setDice;
                        break;
                    }
                    {
                        const golarion::DamageDiceView current = selectedSet->toView();
                        const golarion::DamageDiceView candidate = adjustment.setDice->toView();
                        const long long currentAverageTwice = static_cast<long long>(current.diceCount) * (current.dieSize + 1);
                        const long long candidateAverageTwice = static_cast<long long>(candidate.diceCount) * (candidate.dieSize + 1);
                        if (candidateAverageTwice > currentAverageTwice)
                        {
                            selectedSet = adjustment.setDice;
                        }
                    }
                    break;
            }
        }
        if (progressionSteps < std::numeric_limits<int>::min() || progressionSteps > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("damage dice progression steps are out of range");
        }
        if (combinedMultiplier > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("damage dice count multiplier is out of range");
        }

        golarion::DamageDice dice = selectedSet.value_or(baseDice);
        dice = dice.adjustedByProgression(static_cast<int>(progressionSteps));
        return dice.multiplied(static_cast<int>(combinedMultiplier));
    }

    golarion::CriticalProfileView resolveCriticalProfile(int baseThreatMinimum, int baseMultiplier, const std::vector<ResolvedCriticalAdjustment> &adjustments)
    {
        int threatRangeMultiplier = 1;
        int directThreatMinimum = baseThreatMinimum;
        int multiplierSet = baseMultiplier;
        long long multiplierIncrease = 0;
        std::optional<int> multiplierMaximum;

        for (const ResolvedCriticalAdjustment &adjustment : adjustments)
        {
            switch (adjustment.type)
            {
                case golarion::CriticalAdjustmentType::ThreatRangeMultiplier:
                    if (adjustment.value < 1)
                    {
                        throw std::invalid_argument("critical threat range multiplier must be at least 1");
                    }
                    threatRangeMultiplier = std::max(threatRangeMultiplier, adjustment.value);
                    break;
                case golarion::CriticalAdjustmentType::ThreatMinimum:
                    if (adjustment.value < 1 || adjustment.value > 20)
                    {
                        throw std::invalid_argument("critical threat minimum must be between 1 and 20");
                    }
                    directThreatMinimum = std::min(directThreatMinimum, adjustment.value);
                    break;
                case golarion::CriticalAdjustmentType::MultiplierIncrease:
                    if (adjustment.value < 0)
                    {
                        throw std::invalid_argument("critical multiplier increase must not be negative");
                    }
                    multiplierIncrease += adjustment.value;
                    if (adjustment.maximumMultiplier.has_value())
                    {
                        multiplierMaximum = multiplierMaximum.has_value() ? std::min(*multiplierMaximum, *adjustment.maximumMultiplier) : adjustment.maximumMultiplier;
                    }
                    break;
                case golarion::CriticalAdjustmentType::MultiplierSet:
                    if (adjustment.value < 2)
                    {
                        throw std::invalid_argument("critical multiplier must be at least 2");
                    }
                    multiplierSet = std::max(multiplierSet, adjustment.value);
                    break;
            }
        }

        const long long baseThreatRange = 21LL - baseThreatMinimum;
        const int multipliedThreatMinimum = std::max(1, checkedAttackValue(21LL - baseThreatRange * threatRangeMultiplier));
        long long multiplier = static_cast<long long>(multiplierSet) + multiplierIncrease;
        if (multiplierMaximum.has_value())
        {
            multiplier = std::min(multiplier, static_cast<long long>(*multiplierMaximum));
        }

        return golarion::CriticalProfileView{
            .threatMinimum = std::min(directThreatMinimum, multipliedThreatMinimum),
            .multiplier = checkedAttackValue(multiplier)
        };
    }
}

namespace golarion
{
    std::string_view displayName(AttackMode mode)
    {
        switch (mode)
        {
            case AttackMode::Melee:
                return "Mischia";
            case AttackMode::Ranged:
                return "Distanza";
        }

        throw std::invalid_argument("unknown attack mode");
    }

    std::string_view displayName(AttackTag tag)
    {
        switch (tag)
        {
            case AttackTag::Weapon:
                return "Arma";
            case AttackTag::Thrown:
                return "Lancio";
            case AttackTag::Projectile:
                return "Proiettile";
            case AttackTag::Natural:
                return "Naturale";
            case AttackTag::Unarmed:
                return "Senz'armi";
            case AttackTag::Primary:
                return "Primario";
            case AttackTag::Secondary:
                return "Secondario";
        }

        throw std::invalid_argument("unknown attack tag");
    }

    std::string_view displayName(DamageAbilityRule rule)
    {
        switch (rule)
        {
            case DamageAbilityRule::None:
                return "Nessun modificatore di caratteristica";
            case DamageAbilityRule::Full:
                return "Modificatore completo";
            case DamageAbilityRule::HalfPositiveFullPenalty:
                return "Metà bonus, penalità completa";
            case DamageAbilityRule::OneAndHalfPositiveFullPenalty:
                return "Una volta e mezzo il bonus, penalità completa";
            case DamageAbilityRule::PenaltyOnly:
                return "Solo penalità";
        }

        throw std::invalid_argument("unknown damage ability rule");
    }

    std::string_view displayName(CriticalAdjustmentType type)
    {
        switch (type)
        {
            case CriticalAdjustmentType::ThreatRangeMultiplier:
                return "Moltiplicatore dell'intervallo di minaccia";
            case CriticalAdjustmentType::ThreatMinimum:
                return "Soglia minima di minaccia";
            case CriticalAdjustmentType::MultiplierIncrease:
                return "Aumento del moltiplicatore del critico";
            case CriticalAdjustmentType::MultiplierSet:
                return "Moltiplicatore del critico impostato";
        }

        throw std::invalid_argument("unknown critical adjustment type");
    }

    AbilityType defaultAttackAbility(AttackMode mode)
    {
        switch (mode)
        {
            case AttackMode::Melee:
                return AbilityType::Strength;
            case AttackMode::Ranged:
                return AbilityType::Dexterity;
        }

        throw std::invalid_argument("unknown attack mode");
    }

    std::string attackResourceName(AttackMode mode)
    {
        return categoryResourceName("attack", mode);
    }

    std::string attackResourceName(AttackTag tag)
    {
        return categoryResourceName("attack", tag);
    }

    std::string attackResourceName(std::string_view grantId)
    {
        return grantResourceName("attack", grantId);
    }

    std::string damageResourceName(AttackMode mode)
    {
        return categoryResourceName("damage", mode);
    }

    std::string damageResourceName(AttackTag tag)
    {
        return categoryResourceName("damage", tag);
    }

    std::string damageResourceName(std::string_view grantId)
    {
        return grantResourceName("damage", grantId);
    }

    std::string criticalConfirmationResourceName(AttackMode mode)
    {
        return categoryResourceName("criticalConfirmation", mode);
    }

    std::string criticalConfirmationResourceName(AttackTag tag)
    {
        return categoryResourceName("criticalConfirmation", tag);
    }

    std::string criticalConfirmationResourceName(std::string_view grantId)
    {
        return grantResourceName("criticalConfirmation", grantId);
    }

    AttackGrant::AttackGrant(AttackGrantDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          name_(normalize(definition.name)),
          mode_(definition.mode),
          tags_(std::move(definition.tags)),
          damageComponents_(std::move(definition.damageComponents)),
          damageAbilityRule_(definition.damageAbilityRule),
          criticalThreatMinimum_(definition.criticalThreatMinimum),
          criticalMultiplier_(definition.criticalMultiplier),
          defenseType_(definition.defenseType),
          reach_(definition.reach),
          range_(definition.range),
          requirements_(std::move(definition.requirements))
    {
        std::vector<AttackTag> uniqueTags = tags_;
        std::ranges::sort(uniqueTags);
        if (std::ranges::adjacent_find(uniqueTags) != uniqueTags.end())
        {
            throw std::invalid_argument("attack grant tags must not contain duplicates");
        }
        if ((contains(tags_, AttackTag::Thrown) || contains(tags_, AttackTag::Projectile)) && mode_ != AttackMode::Ranged)
        {
            throw std::invalid_argument("thrown and projectile attacks must use ranged attack mode");
        }
        if (contains(tags_, AttackTag::Primary) && contains(tags_, AttackTag::Secondary))
        {
            throw std::invalid_argument("an attack cannot be both primary and secondary");
        }
        if ((contains(tags_, AttackTag::Primary) || contains(tags_, AttackTag::Secondary)) && !contains(tags_, AttackTag::Natural))
        {
            throw std::invalid_argument("primary and secondary tags are only valid for natural attacks");
        }
        std::vector<std::string> damageComponentIds;
        damageComponentIds.reserve(damageComponents_.size());
        for (const DamageComponent &component : damageComponents_)
        {
            damageComponentIds.push_back(component.id_);
        }
        std::ranges::sort(damageComponentIds);
        if (std::ranges::adjacent_find(damageComponentIds) != damageComponentIds.end())
        {
            throw std::invalid_argument("attack grant damage component IDs must be unique");
        }
        if (criticalThreatMinimum_ < 1 || criticalThreatMinimum_ > 20)
        {
            throw std::invalid_argument("critical threat minimum must be between 1 and 20");
        }
        if (criticalMultiplier_ < 2)
        {
            throw std::invalid_argument("critical multiplier must be at least 2");
        }
        if (mode_ == AttackMode::Melee && !reach_.has_value())
        {
            throw std::invalid_argument("melee attacks require a reach profile");
        }
        if (mode_ == AttackMode::Melee && range_.has_value())
        {
            throw std::invalid_argument("melee attacks must not have a range profile");
        }
        if (mode_ == AttackMode::Ranged && reach_.has_value())
        {
            throw std::invalid_argument("ranged attacks must not have a reach profile");
        }
        if (reach_.has_value() && (reach_->minimumUnits < 0 || reach_->maximumUnits < reach_->minimumUnits))
        {
            throw std::invalid_argument("attack reach must have a non-negative ordered interval");
        }
        if (range_.has_value() && (range_->incrementUnits <= 0 || range_->maximumIncrements <= 0 || range_->penaltyPerAdditionalIncrement > 0))
        {
            throw std::invalid_argument("attack range must have positive distances and a non-positive penalty");
        }
    }

    DamageComponentGrant::DamageComponentGrant(DamageComponentGrantDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          component_(std::move(definition.component))
    {
    }

    AttackRequirement::AttackRequirement(AttackRequirementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          requirement_(std::move(definition.requirement)),
          condition_(std::move(definition.condition))
    {
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }
    }

    AttackDefenseReplacement::AttackDefenseReplacement(AttackDefenseReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          defenseType_(definition.defenseType),
          condition_(std::move(definition.condition))
    {
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }
    }

    CriticalAdjustment::CriticalAdjustment(CriticalAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          type_(definition.type),
          expression_(normalize(definition.expression)),
          maximumMultiplier_(definition.maximumMultiplier),
          condition_(std::move(definition.condition))
    {
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }
        if (maximumMultiplier_.has_value() && *maximumMultiplier_ < 2)
        {
            throw std::invalid_argument("maximum critical multiplier must be at least 2");
        }
        if (maximumMultiplier_.has_value() && type_ != CriticalAdjustmentType::MultiplierIncrease)
        {
            throw std::invalid_argument("maximum critical multiplier is only valid for multiplier increases");
        }
    }

    AttackAbilityReplacement::AttackAbilityReplacement(AttackAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          abilityType_(definition.abilityType)
    {
    }

    DamageAbilityReplacement::DamageAbilityReplacement(DamageAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          abilityType_(definition.abilityType)
    {
    }

    DamageDiceAdjustment::DamageDiceAdjustment(DamageDiceAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          targetRole_(definition.targetRole),
          targetComponentId_(std::move(definition.targetComponentId)),
          type_(definition.type),
          expression_(std::move(definition.expression)),
          setDice_(std::move(definition.setDice)),
          condition_(std::move(definition.condition))
    {
        if (targetComponentId_.has_value())
        {
            targetComponentId_ = normalize(*targetComponentId_);
        }
        if (expression_.has_value())
        {
            expression_ = normalize(*expression_);
        }
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }
        if (type_ == DamageDiceAdjustmentType::Set && (!setDice_.has_value() || expression_.has_value()))
        {
            throw std::invalid_argument("set damage dice adjustments require dice and must not have an expression");
        }
        if (type_ != DamageDiceAdjustmentType::Set && (!expression_.has_value() || setDice_.has_value()))
        {
            throw std::invalid_argument("calculated damage dice adjustments require an expression and must not set dice");
        }
    }

    Attacks::Attacks(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource("attack.all");
        resourceManager_.registerEnhanceableResource("damage.all");
        resourceManager_.registerEnhanceableResource("criticalConfirmation.all");

        for (AttackDistanceProperty property : AttackDistanceProperties)
        {
            resourceManager_.registerEnhanceableResource(attackDistanceResourceName(property));
        }

        for (AttackMode mode : {AttackMode::Melee, AttackMode::Ranged})
        {
            resourceManager_.registerEnhanceableResource(attackResourceName(mode), {"attack.all"});
            resourceManager_.registerEnhanceableResource(damageResourceName(mode), {"damage.all"});
            resourceManager_.registerEnhanceableResource(criticalConfirmationResourceName(mode), {"criticalConfirmation.all"});
            for (AttackDistanceProperty property : AttackDistanceProperties)
            {
                resourceManager_.registerEnhanceableResource(attackDistanceResourceName(property, mode), {attackDistanceResourceName(property)});
            }
        }
        for (AttackTag tag : {AttackTag::Weapon, AttackTag::Thrown, AttackTag::Projectile, AttackTag::Natural, AttackTag::Unarmed, AttackTag::Primary, AttackTag::Secondary})
        {
            resourceManager_.registerEnhanceableResource(attackResourceName(tag), {"attack.all"});
            resourceManager_.registerEnhanceableResource(damageResourceName(tag), {"damage.all"});
            resourceManager_.registerEnhanceableResource(criticalConfirmationResourceName(tag), {"criticalConfirmation.all"});
            for (AttackDistanceProperty property : AttackDistanceProperties)
            {
                resourceManager_.registerEnhanceableResource(attackDistanceResourceName(property, tag), {attackDistanceResourceName(property)});
            }
        }

        resourceManager_.registerCollectionResource<AttackGrant>(AttackGrantsResource, [this](AttackGrant grant)
        {
            addGrant(std::move(grant));
        }, [this](std::string_view grantId)
        {
            removeGrant(grantId);
        });
        resourceManager_.registerCollectionResource<DamageComponentGrant>(DamageComponentGrantsResource, [this](DamageComponentGrant grant)
        {
            addDamageComponentGrant(std::move(grant));
        }, [this](std::string_view grantId)
        {
            removeDamageComponentGrant(grantId);
        });
        resourceManager_.registerCollectionResource<CriticalAdjustment>(CriticalAdjustmentsResource, [this](CriticalAdjustment adjustment)
        {
            addCriticalAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeCriticalAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<AttackAbilityReplacement>(AttackAbilityReplacementsResource, [this](AttackAbilityReplacement replacement)
        {
            addAttackAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeAttackAbilityReplacement(replacementId);
        });
        resourceManager_.registerCollectionResource<DamageAbilityReplacement>(DamageAbilityReplacementsResource, [this](DamageAbilityReplacement replacement)
        {
            addDamageAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeDamageAbilityReplacement(replacementId);
        });
        resourceManager_.registerCollectionResource<DamageDiceAdjustment>(DamageDiceAdjustmentsResource, [this](DamageDiceAdjustment adjustment)
        {
            addDamageDiceAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeDamageDiceAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<AttackDistanceAdjustment>(AttackDistanceAdjustmentsResource, [this](AttackDistanceAdjustment adjustment)
        {
            addDistanceAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeDistanceAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<AttackRequirement>(AttackRequirementsResource, [this](AttackRequirement requirement)
        {
            addRequirement(std::move(requirement));
        }, [this](std::string_view requirementId)
        {
            removeRequirement(requirementId);
        });
        resourceManager_.registerCollectionResource<AttackDefenseReplacement>(AttackDefenseReplacementsResource, [this](AttackDefenseReplacement replacement)
        {
            addDefenseReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeDefenseReplacement(replacementId);
        });
    }

    AttacksView Attacks::toView()
    {
        const int baseAttackBonus = resourceManager_.targetValue("bab");
        std::vector<AttackView> attackViews;
        attackViews.reserve(grants_.size());

        for (const AttackGrant &grant : grants_)
        {
            const std::string attackResource = attackResourceName(grant.id_);
            const std::string damageResource = damageResourceName(grant.id_);
            ModifierSetView attackModifiers = resourceManager_.modifierSetView(attackResource);
            const int naturalSecondaryPenalty = contains(grant.tags_, AttackTag::Secondary) ? -5 : 0;
            ModifierSetView confirmationModifiers = resourceManager_.modifierSetView(criticalConfirmationResourceName(grant.id_));
            std::vector<AttackDefenseOptionView> defenseOptions;
            defenseOptions.reserve(defenseReplacements_.size() + 1);
            defenseOptions.push_back(AttackDefenseOptionView{
                .replacementId = std::nullopt,
                .source = "Base",
                .defenseType = grant.defenseType_,
                .condition = std::nullopt
            });
            for (const auto &[id, replacement] : defenseReplacements_)
            {
                if (resourceManager_.enhanceableResourceIsOrInheritsFrom(attackResource, replacement.targetResourceName_))
                {
                    defenseOptions.push_back(AttackDefenseOptionView{
                        .replacementId = id,
                        .source = replacement.source_,
                        .defenseType = replacement.defenseType_,
                        .condition = replacement.condition_
                    });
                }
            }
            const auto attackAbilityOption = [&](std::optional<std::string> replacementId, const std::string &source, AbilityType abilityType)
            {
                const int abilityModifier = resourceManager_.targetValue(std::string(resourceName(abilityType)) + "Mod");
                const int firstAttackBonus = checkedAttackValue(static_cast<long long>(baseAttackBonus) + abilityModifier + naturalSecondaryPenalty + attackModifiers.total);
                std::vector<int> attackBonuses{firstAttackBonus};
                if (!contains(grant.tags_, AttackTag::Natural))
                {
                    for (int iterativeOffset = 5; baseAttackBonus - iterativeOffset > 0; iterativeOffset += 5)
                    {
                        attackBonuses.push_back(checkedAttackValue(static_cast<long long>(firstAttackBonus) - iterativeOffset));
                    }
                }
                return AttackAbilityOptionView{
                    .replacementId = std::move(replacementId),
                    .source = source,
                    .abilityType = abilityType,
                    .abilityModifier = abilityModifier,
                    .attackBonuses = std::move(attackBonuses),
                    .criticalConfirmationBonus = checkedAttackValue(static_cast<long long>(baseAttackBonus) + abilityModifier + naturalSecondaryPenalty + confirmationModifiers.total)
                };
            };
            std::vector<AttackAbilityOptionView> attackAbilityOptions;
            attackAbilityOptions.reserve(attackAbilityReplacements_.size() + 1);
            attackAbilityOptions.push_back(attackAbilityOption(std::nullopt, "Base", defaultAttackAbility(grant.mode_)));
            for (const auto &[id, replacement] : attackAbilityReplacements_)
            {
                if (resourceManager_.enhanceableResourceIsOrInheritsFrom(attackResource, replacement.targetResourceName_))
                {
                    attackAbilityOptions.push_back(attackAbilityOption(id, replacement.source_, replacement.abilityType_));
                }
            }

            std::vector<ResolvedCriticalAdjustment> permanentCriticalAdjustments;
            std::map<std::string, std::vector<ResolvedCriticalAdjustment>> conditionalCriticalAdjustments;
            std::vector<CriticalAdjustmentView> criticalAdjustmentViews;
            for (const auto &[id, adjustment] : criticalAdjustments_)
            {
                if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(attackResource, adjustment.targetResourceName_))
                {
                    continue;
                }

                const int resolvedValue = resourceManager_.evaluateExpression(adjustment.expression_);
                const ResolvedCriticalAdjustment resolved{
                    .type = adjustment.type_,
                    .value = resolvedValue,
                    .maximumMultiplier = adjustment.maximumMultiplier_
                };
                criticalAdjustmentViews.push_back(CriticalAdjustmentView{
                    .id = id,
                    .source = adjustment.source_,
                    .targetResourceName = adjustment.targetResourceName_,
                    .type = adjustment.type_,
                    .expression = adjustment.expression_,
                    .resolvedValue = resolvedValue,
                    .maximumMultiplier = adjustment.maximumMultiplier_,
                    .condition = adjustment.condition_
                });
                if (adjustment.condition_.has_value())
                {
                    conditionalCriticalAdjustments[*adjustment.condition_].push_back(resolved);
                }
                else
                {
                    permanentCriticalAdjustments.push_back(resolved);
                }
            }
            const CriticalProfileView criticalProfile = resolveCriticalProfile(grant.criticalThreatMinimum_, grant.criticalMultiplier_, permanentCriticalAdjustments);
            std::vector<ConditionalCriticalProfileView> conditionalCriticalProfiles;
            conditionalCriticalProfiles.reserve(conditionalCriticalAdjustments.size());
            for (const auto &[condition, conditionalAdjustments] : conditionalCriticalAdjustments)
            {
                std::vector<ResolvedCriticalAdjustment> applicableAdjustments = permanentCriticalAdjustments;
                applicableAdjustments.insert(applicableAdjustments.end(), conditionalAdjustments.begin(), conditionalAdjustments.end());
                conditionalCriticalProfiles.push_back(ConditionalCriticalProfileView{
                    .condition = condition,
                    .profile = resolveCriticalProfile(grant.criticalThreatMinimum_, grant.criticalMultiplier_, applicableAdjustments)
                });
            }

            std::vector<DamageComponentView> damageComponents;
            damageComponents.reserve(grant.damageComponents_.size() + damageComponentGrants_.size());
            const auto appendDamageComponent = [this, &damageComponents, &criticalProfile, &damageResource](const DamageComponent &component, std::optional<std::string> componentGrantId, std::optional<std::string> componentGrantSource)
            {
                std::vector<ResolvedDamageDiceAdjustment> permanentAdjustments;
                std::map<std::string, std::vector<ResolvedDamageDiceAdjustment>> conditionalAdjustments;
                std::vector<DamageDiceAdjustmentView> adjustmentViews;
                for (const auto &[id, adjustment] : damageDiceAdjustments_)
                {
                    if (adjustment.targetRole_ != component.role_ || (adjustment.targetComponentId_.has_value() && *adjustment.targetComponentId_ != component.id_) || !resourceManager_.enhanceableResourceIsOrInheritsFrom(damageResource, adjustment.targetResourceName_))
                    {
                        continue;
                    }
                    std::optional<int> resolvedValue;
                    if (adjustment.expression_.has_value())
                    {
                        resolvedValue = resourceManager_.evaluateExpression(*adjustment.expression_);
                    }
                    const ResolvedDamageDiceAdjustment resolved{
                        .type = adjustment.type_,
                        .value = resolvedValue,
                        .setDice = adjustment.setDice_
                    };
                    adjustmentViews.push_back(DamageDiceAdjustmentView{
                        .id = id,
                        .source = adjustment.source_,
                        .targetResourceName = adjustment.targetResourceName_,
                        .targetRole = adjustment.targetRole_,
                        .targetComponentId = adjustment.targetComponentId_,
                        .type = adjustment.type_,
                        .expression = adjustment.expression_,
                        .resolvedValue = resolvedValue,
                        .setDice = adjustment.setDice_.has_value() ? std::optional<DamageDiceView>(adjustment.setDice_->toView()) : std::nullopt,
                        .condition = adjustment.condition_
                    });
                    if (adjustment.condition_.has_value())
                    {
                        conditionalAdjustments[*adjustment.condition_].push_back(resolved);
                    }
                    else
                    {
                        permanentAdjustments.push_back(resolved);
                    }
                }
                const DamageDice effectiveDice = resolveDamageDice(component.dice_, permanentAdjustments);
                std::vector<ConditionalDamageDiceView> conditionalDice;
                conditionalDice.reserve(conditionalAdjustments.size());
                for (const auto &[condition, conditionAdjustments] : conditionalAdjustments)
                {
                    std::vector<ResolvedDamageDiceAdjustment> applicableAdjustments = permanentAdjustments;
                    applicableAdjustments.insert(applicableAdjustments.end(), conditionAdjustments.begin(), conditionAdjustments.end());
                    conditionalDice.push_back(ConditionalDamageDiceView{
                        .condition = condition,
                        .dice = resolveDamageDice(component.dice_, applicableAdjustments).toView()
                    });
                }
                damageComponents.push_back(DamageComponentView{
                    .id = component.id_,
                    .source = component.source_,
                    .grantId = std::move(componentGrantId),
                    .grantSource = std::move(componentGrantSource),
                    .role = component.role_,
                    .baseDice = component.dice_.toView(),
                    .effectiveDice = effectiveDice.toView(),
                    .conditionalDice = std::move(conditionalDice),
                    .diceAdjustments = std::move(adjustmentViews),
                    .types = component.types_,
                    .typeMode = component.typeMode_,
                    .criticalRule = component.criticalRule_,
                    .traits = component.traits_,
                    .includedInNormalDamage = component.criticalRule_ != DamageCriticalRule::CriticalOnly,
                    .criticalOccurrences = component.criticalRule_ == DamageCriticalRule::Multiplied ? criticalProfile.multiplier : 1
                });
            };
            for (const DamageComponent &component : grant.damageComponents_)
            {
                appendDamageComponent(component, std::nullopt, std::nullopt);
            }
            for (const auto &[id, componentGrant] : damageComponentGrants_)
            {
                if (resourceManager_.enhanceableResourceIsOrInheritsFrom(damageResource, componentGrant.targetResourceName_))
                {
                    appendDamageComponent(componentGrant.component_, id, componentGrant.source_);
                }
            }

            ModifierSetView damageModifiers = resourceManager_.modifierSetView(damageResource);
            const auto damageAbilityOption = [&](std::optional<std::string> replacementId, const std::string &source, AbilityType abilityType)
            {
                const int abilityModifier = resourceManager_.targetValue(std::string(resourceName(abilityType)) + "Mod");
                const int abilityContribution = damageAbilityContribution(abilityModifier, grant.damageAbilityRule_);
                const int damageBonus = checkedAttackValue(static_cast<long long>(abilityContribution) + damageModifiers.total);
                return DamageAbilityOptionView{
                    .replacementId = std::move(replacementId),
                    .source = source,
                    .abilityType = abilityType,
                    .abilityModifier = abilityModifier,
                    .abilityContribution = abilityContribution,
                    .damageBonus = damageBonus,
                    .criticalDamageBonus = checkedAttackValue(static_cast<long long>(damageBonus) * criticalProfile.multiplier)
                };
            };
            std::vector<DamageAbilityOptionView> damageAbilityOptions;
            damageAbilityOptions.reserve(damageAbilityReplacements_.size() + 1);
            damageAbilityOptions.push_back(damageAbilityOption(std::nullopt, "Base", AbilityType::Strength));
            for (const auto &[id, replacement] : damageAbilityReplacements_)
            {
                if (resourceManager_.enhanceableResourceIsOrInheritsFrom(damageResource, replacement.targetResourceName_))
                {
                    damageAbilityOptions.push_back(damageAbilityOption(id, replacement.source_, replacement.abilityType_));
                }
            }

            const auto distanceValueView = [this, &grant](AttackDistanceProperty property, int baseValue)
            {
                const std::string resourceName = attackDistanceResourceName(property, grant.id_);
                ModifierSetView modifiers = resourceManager_.modifierSetView(resourceName);
                std::vector<ResolvedAttackDistanceAdjustment> permanentAdjustments;
                std::map<std::string, std::vector<ResolvedAttackDistanceAdjustment>> conditionalAdjustments;
                std::vector<AttackDistanceAdjustmentView> adjustmentViews;
                for (const auto &[id, adjustment] : distanceAdjustments_)
                {
                    if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(resourceName, adjustment.targetResourceName_))
                    {
                        continue;
                    }
                    const int resolvedValue = resourceManager_.evaluateExpression(adjustment.expression_);
                    const ResolvedAttackDistanceAdjustment resolved{
                        .type = adjustment.type_,
                        .value = resolvedValue
                    };
                    adjustmentViews.push_back(AttackDistanceAdjustmentView{
                        .id = id,
                        .source = adjustment.source_,
                        .targetResourceName = adjustment.targetResourceName_,
                        .type = adjustment.type_,
                        .expression = adjustment.expression_,
                        .resolvedValue = resolvedValue,
                        .condition = adjustment.condition_
                    });
                    if (adjustment.condition_.has_value())
                    {
                        conditionalAdjustments[*adjustment.condition_].push_back(resolved);
                    }
                    else
                    {
                        permanentAdjustments.push_back(resolved);
                    }
                }

                std::map<std::string, int> conditionalModifierTotals;
                for (const ModifierSetView::ConditionalTotalView &conditionalTotal : modifiers.conditionalTotals)
                {
                    conditionalModifierTotals.emplace(conditionalTotal.condition, conditionalTotal.value);
                }
                for (const auto &[condition, adjustments] : conditionalAdjustments)
                {
                    conditionalModifierTotals.try_emplace(condition, 0);
                }

                std::vector<ConditionalAttackDistanceValueView> conditionalValues;
                conditionalValues.reserve(conditionalModifierTotals.size());
                for (const auto &[condition, conditionalModifierTotal] : conditionalModifierTotals)
                {
                    std::vector<ResolvedAttackDistanceAdjustment> applicableAdjustments = permanentAdjustments;
                    const auto conditionAdjustments = conditionalAdjustments.find(condition);
                    if (conditionAdjustments != conditionalAdjustments.end())
                    {
                        applicableAdjustments.insert(applicableAdjustments.end(), conditionAdjustments->second.begin(), conditionAdjustments->second.end());
                    }
                    conditionalValues.push_back(ConditionalAttackDistanceValueView{
                        .condition = condition,
                        .value = resolveAttackDistanceValue(baseValue, checkedAttackValue(static_cast<long long>(modifiers.total) + conditionalModifierTotal), applicableAdjustments)
                    });
                }

                return AttackDistanceValueView{
                    .resourceName = resourceName,
                    .baseValue = baseValue,
                    .effectiveValue = resolveAttackDistanceValue(baseValue, modifiers.total, permanentAdjustments),
                    .conditionalValues = std::move(conditionalValues),
                    .modifiers = std::move(modifiers),
                    .adjustments = std::move(adjustmentViews)
                };
            };

            std::optional<AttackReachView> reach;
            if (grant.reach_.has_value())
            {
                reach = AttackReachView{
                    .minimumUnits = distanceValueView(AttackDistanceProperty::MinimumReach, grant.reach_->minimumUnits),
                    .maximumUnits = distanceValueView(AttackDistanceProperty::MaximumReach, grant.reach_->maximumUnits)
                };
                if (reach->minimumUnits.effectiveValue < 0 || reach->maximumUnits.effectiveValue < reach->minimumUnits.effectiveValue)
                {
                    throw std::invalid_argument("effective attack reach must have a non-negative ordered interval");
                }
            }

            std::optional<AttackRangeView> range;
            if (grant.range_.has_value())
            {
                range = AttackRangeView{
                    .incrementUnits = distanceValueView(AttackDistanceProperty::RangeIncrement, grant.range_->incrementUnits),
                    .maximumIncrements = distanceValueView(AttackDistanceProperty::MaximumRangeIncrements, grant.range_->maximumIncrements),
                    .penaltyPerAdditionalIncrement = distanceValueView(AttackDistanceProperty::RangePenaltyPerAdditionalIncrement, grant.range_->penaltyPerAdditionalIncrement)
                };
                if (range->incrementUnits.effectiveValue <= 0 || range->maximumIncrements.effectiveValue <= 0 || range->penaltyPerAdditionalIncrement.effectiveValue > 0)
                {
                    throw std::invalid_argument("effective attack range must have positive distances and a non-positive penalty");
                }
            }

            std::vector<AttackRequirementView> requirementViews;
            requirementViews.reserve(grant.requirements_.size() + requirements_.size());
            std::vector<std::string> failureReasons;
            std::map<std::string, std::vector<std::string>> conditionalFailureReasons;
            for (const Requirement &requirement : grant.requirements_)
            {
                RequirementView requirementView = requirement.toView(resourceManager_);
                if (!requirementView.satisfied)
                {
                    failureReasons.push_back(requirementView.failureReason);
                }
                requirementViews.push_back(AttackRequirementView{
                    .id = std::nullopt,
                    .source = grant.source_,
                    .targetResourceName = std::nullopt,
                    .requirement = std::move(requirementView),
                    .condition = std::nullopt
                });
            }
            for (const auto &[id, attackRequirement] : requirements_)
            {
                if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(attackResource, attackRequirement.targetResourceName_))
                {
                    continue;
                }
                RequirementView requirementView = attackRequirement.requirement_.toView(resourceManager_);
                if (attackRequirement.condition_.has_value())
                {
                    std::vector<std::string> &reasons = conditionalFailureReasons[*attackRequirement.condition_];
                    if (!requirementView.satisfied)
                    {
                        reasons.push_back(requirementView.failureReason);
                    }
                }
                else if (!requirementView.satisfied)
                {
                    failureReasons.push_back(requirementView.failureReason);
                }
                requirementViews.push_back(AttackRequirementView{
                    .id = id,
                    .source = attackRequirement.source_,
                    .targetResourceName = attackRequirement.targetResourceName_,
                    .requirement = std::move(requirementView),
                    .condition = attackRequirement.condition_
                });
            }

            std::vector<ConditionalAttackUsabilityView> conditionalUsability;
            conditionalUsability.reserve(conditionalFailureReasons.size());
            for (auto &[condition, conditionalReasons] : conditionalFailureReasons)
            {
                std::vector<std::string> reasons = failureReasons;
                reasons.insert(reasons.end(), conditionalReasons.begin(), conditionalReasons.end());
                conditionalUsability.push_back(ConditionalAttackUsabilityView{
                    .condition = condition,
                    .usable = reasons.empty(),
                    .failureReasons = std::move(reasons)
                });
            }
            attackViews.push_back(AttackView{
                .id = grant.id_,
                .source = grant.source_,
                .name = grant.name_,
                .mode = grant.mode_,
                .tags = grant.tags_,
                .defenseOptions = std::move(defenseOptions),
                .attackAbilityOptions = std::move(attackAbilityOptions),
                .attackModifiers = std::move(attackModifiers),
                .criticalConfirmationModifiers = std::move(confirmationModifiers),
                .criticalProfile = criticalProfile,
                .conditionalCriticalProfiles = std::move(conditionalCriticalProfiles),
                .criticalAdjustments = std::move(criticalAdjustmentViews),
                .damageAbilityRule = grant.damageAbilityRule_,
                .damageAbilityOptions = std::move(damageAbilityOptions),
                .damageModifiers = std::move(damageModifiers),
                .damageComponents = std::move(damageComponents),
                .reach = std::move(reach),
                .range = std::move(range),
                .usable = failureReasons.empty(),
                .failureReasons = std::move(failureReasons),
                .conditionalUsability = std::move(conditionalUsability),
                .requirements = std::move(requirementViews)
            });
        }

        return AttacksView{.attacks = std::move(attackViews)};
    }

    void Attacks::addGrant(AttackGrant grant)
    {
        const auto duplicate = std::ranges::find(grants_, grant.id_, &AttackGrant::id_);
        if (duplicate != grants_.end())
        {
            throw std::invalid_argument("attack grant is already registered: " + grant.id_);
        }

        std::vector<std::pair<std::string, std::vector<std::string>>> resources{
            {attackResourceName(grant.id_), attackParentResources(grant)},
            {damageResourceName(grant.id_), damageParentResources(grant)},
            {criticalConfirmationResourceName(grant.id_), criticalConfirmationParentResources(grant)}
        };
        if (grant.reach_.has_value())
        {
            for (AttackDistanceProperty property : {AttackDistanceProperty::MinimumReach, AttackDistanceProperty::MaximumReach})
            {
                resources.emplace_back(attackDistanceResourceName(property, grant.id_), distanceParentResources(property, grant));
            }
        }
        if (grant.range_.has_value())
        {
            for (AttackDistanceProperty property : {AttackDistanceProperty::RangeIncrement, AttackDistanceProperty::MaximumRangeIncrements, AttackDistanceProperty::RangePenaltyPerAdditionalIncrement})
            {
                resources.emplace_back(attackDistanceResourceName(property, grant.id_), distanceParentResources(property, grant));
            }
        }

        std::size_t registeredCount = 0;
        try
        {
            for (const auto &[resourceName, parents] : resources)
            {
                resourceManager_.registerEnhanceableResource(resourceName, parents);
                ++registeredCount;
            }
        }
        catch (...)
        {
            while (registeredCount > 0)
            {
                --registeredCount;
                resourceManager_.unregisterEnhanceableResource(resources[registeredCount].first);
            }
            throw;
        }
        grants_.push_back(std::move(grant));
    }

    void Attacks::removeGrant(std::string_view grantId)
    {
        const std::string normalizedId = normalize(grantId);
        const auto grant = std::ranges::find(grants_, normalizedId, &AttackGrant::id_);
        if (grant == grants_.end())
        {
            throw std::invalid_argument("attack grant is not registered: " + normalizedId);
        }

        const std::string attackResource = attackResourceName(normalizedId);
        const std::string damageResource = damageResourceName(normalizedId);
        const std::string criticalConfirmationResource = criticalConfirmationResourceName(normalizedId);
        const auto dependentDamageComponent = std::ranges::find_if(damageComponentGrants_, [&damageResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == damageResource;
        });
        if (dependentDamageComponent != damageComponentGrants_.end())
        {
            throw std::invalid_argument("attack grant is targeted by damage component grant: " + dependentDamageComponent->first);
        }
        const auto dependentCriticalAdjustment = std::ranges::find_if(criticalAdjustments_, [&attackResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == attackResource;
        });
        if (dependentCriticalAdjustment != criticalAdjustments_.end())
        {
            throw std::invalid_argument("attack grant is targeted by critical adjustment: " + dependentCriticalAdjustment->first);
        }
        const auto dependentAttackAbilityReplacement = std::ranges::find_if(attackAbilityReplacements_, [&attackResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == attackResource;
        });
        if (dependentAttackAbilityReplacement != attackAbilityReplacements_.end())
        {
            throw std::invalid_argument("attack grant is targeted by attack ability replacement: " + dependentAttackAbilityReplacement->first);
        }
        const auto dependentDamageAbilityReplacement = std::ranges::find_if(damageAbilityReplacements_, [&damageResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == damageResource;
        });
        if (dependentDamageAbilityReplacement != damageAbilityReplacements_.end())
        {
            throw std::invalid_argument("attack grant is targeted by damage ability replacement: " + dependentDamageAbilityReplacement->first);
        }
        const auto dependentDamageDiceAdjustment = std::ranges::find_if(damageDiceAdjustments_, [&damageResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == damageResource;
        });
        if (dependentDamageDiceAdjustment != damageDiceAdjustments_.end())
        {
            throw std::invalid_argument("attack grant is targeted by damage dice adjustment: " + dependentDamageDiceAdjustment->first);
        }
        const auto dependentDistanceAdjustment = std::ranges::find_if(distanceAdjustments_, [&normalizedId](const auto &entry)
        {
            return std::ranges::any_of(AttackDistanceProperties, [&entry, &normalizedId](AttackDistanceProperty property)
            {
                return entry.second.targetResourceName_ == attackDistanceResourceName(property, normalizedId);
            });
        });
        if (dependentDistanceAdjustment != distanceAdjustments_.end())
        {
            throw std::invalid_argument("attack grant is targeted by distance adjustment: " + dependentDistanceAdjustment->first);
        }
        const auto dependentRequirement = std::ranges::find_if(requirements_, [&attackResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == attackResource;
        });
        if (dependentRequirement != requirements_.end())
        {
            throw std::invalid_argument("attack grant is targeted by requirement: " + dependentRequirement->first);
        }
        const auto dependentDefenseReplacement = std::ranges::find_if(defenseReplacements_, [&attackResource](const auto &entry)
        {
            return entry.second.targetResourceName_ == attackResource;
        });
        if (dependentDefenseReplacement != defenseReplacements_.end())
        {
            throw std::invalid_argument("attack grant is targeted by defense replacement: " + dependentDefenseReplacement->first);
        }

        std::vector<std::pair<std::string, std::vector<std::string>>> resources{
            {attackResource, attackParentResources(*grant)},
            {damageResource, damageParentResources(*grant)},
            {criticalConfirmationResource, criticalConfirmationParentResources(*grant)}
        };
        if (grant->reach_.has_value())
        {
            for (AttackDistanceProperty property : {AttackDistanceProperty::MinimumReach, AttackDistanceProperty::MaximumReach})
            {
                resources.emplace_back(attackDistanceResourceName(property, normalizedId), distanceParentResources(property, *grant));
            }
        }
        if (grant->range_.has_value())
        {
            for (AttackDistanceProperty property : {AttackDistanceProperty::RangeIncrement, AttackDistanceProperty::MaximumRangeIncrements, AttackDistanceProperty::RangePenaltyPerAdditionalIncrement})
            {
                resources.emplace_back(attackDistanceResourceName(property, normalizedId), distanceParentResources(property, *grant));
            }
        }

        std::vector<bool> removed(resources.size(), false);
        try
        {
            for (std::size_t index = resources.size(); index > 0; --index)
            {
                resourceManager_.unregisterEnhanceableResource(resources[index - 1].first);
                removed[index - 1] = true;
            }
        }
        catch (...)
        {
            for (std::size_t index = 0; index < resources.size(); ++index)
            {
                if (removed[index])
                {
                    resourceManager_.registerEnhanceableResource(resources[index].first, resources[index].second);
                }
            }
            throw;
        }
        grants_.erase(grant);
    }

    void Attacks::addDamageComponentGrant(DamageComponentGrant grant)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(grant.targetResourceName_, "damage.all"))
        {
            throw std::invalid_argument("damage component grant must target a damage resource: " + grant.targetResourceName_);
        }

        const std::string id = grant.id_;
        if (!damageComponentGrants_.emplace(id, std::move(grant)).second)
        {
            throw std::invalid_argument("damage component grant is already registered: " + id);
        }
    }

    void Attacks::removeDamageComponentGrant(std::string_view grantId)
    {
        const std::string id = normalize(grantId);
        if (damageComponentGrants_.erase(id) == 0)
        {
            throw std::invalid_argument("damage component grant is not registered: " + id);
        }
    }

    void Attacks::addCriticalAdjustment(CriticalAdjustment adjustment)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(adjustment.targetResourceName_, "attack.all"))
        {
            throw std::invalid_argument("critical adjustment must target an attack resource: " + adjustment.targetResourceName_);
        }

        const std::string id = adjustment.id_;
        if (!criticalAdjustments_.emplace(id, std::move(adjustment)).second)
        {
            throw std::invalid_argument("critical adjustment is already registered: " + id);
        }
    }

    void Attacks::removeCriticalAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (criticalAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("critical adjustment is not registered: " + id);
        }
    }

    void Attacks::addAttackAbilityReplacement(AttackAbilityReplacement replacement)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(replacement.targetResourceName_, "attack.all"))
        {
            throw std::invalid_argument("attack ability replacement must target an attack resource: " + replacement.targetResourceName_);
        }
        const std::string id = replacement.id_;
        if (!attackAbilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("attack ability replacement is already registered: " + id);
        }
    }

    void Attacks::removeAttackAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (attackAbilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("attack ability replacement is not registered: " + id);
        }
    }

    void Attacks::addDamageAbilityReplacement(DamageAbilityReplacement replacement)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(replacement.targetResourceName_, "damage.all"))
        {
            throw std::invalid_argument("damage ability replacement must target a damage resource: " + replacement.targetResourceName_);
        }
        const std::string id = replacement.id_;
        if (!damageAbilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("damage ability replacement is already registered: " + id);
        }
    }

    void Attacks::removeDamageAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (damageAbilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("damage ability replacement is not registered: " + id);
        }
    }

    void Attacks::addDamageDiceAdjustment(DamageDiceAdjustment adjustment)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(adjustment.targetResourceName_, "damage.all"))
        {
            throw std::invalid_argument("damage dice adjustment must target a damage resource: " + adjustment.targetResourceName_);
        }
        const std::string id = adjustment.id_;
        if (!damageDiceAdjustments_.emplace(id, std::move(adjustment)).second)
        {
            throw std::invalid_argument("damage dice adjustment is already registered: " + id);
        }
    }

    void Attacks::removeDamageDiceAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (damageDiceAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("damage dice adjustment is not registered: " + id);
        }
    }

    void Attacks::addDistanceAdjustment(AttackDistanceAdjustment adjustment)
    {
        const bool targetsDistance = std::ranges::any_of(AttackDistanceProperties, [this, &adjustment](AttackDistanceProperty property)
        {
            return resourceManager_.enhanceableResourceIsOrInheritsFrom(adjustment.targetResourceName_, attackDistanceResourceName(property));
        });
        if (!targetsDistance)
        {
            throw std::invalid_argument("attack distance adjustment must target an attack distance resource: " + adjustment.targetResourceName_);
        }
        const std::string id = adjustment.id_;
        if (!distanceAdjustments_.emplace(id, std::move(adjustment)).second)
        {
            throw std::invalid_argument("attack distance adjustment is already registered: " + id);
        }
    }

    void Attacks::removeDistanceAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (distanceAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("attack distance adjustment is not registered: " + id);
        }
    }

    void Attacks::addRequirement(AttackRequirement requirement)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(requirement.targetResourceName_, "attack.all"))
        {
            throw std::invalid_argument("attack requirement must target an attack resource: " + requirement.targetResourceName_);
        }
        const std::string id = requirement.id_;
        if (!requirements_.emplace(id, std::move(requirement)).second)
        {
            throw std::invalid_argument("attack requirement is already registered: " + id);
        }
    }

    void Attacks::removeRequirement(std::string_view requirementId)
    {
        const std::string id = normalize(requirementId);
        if (requirements_.erase(id) == 0)
        {
            throw std::invalid_argument("attack requirement is not registered: " + id);
        }
    }

    void Attacks::addDefenseReplacement(AttackDefenseReplacement replacement)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(replacement.targetResourceName_, "attack.all"))
        {
            throw std::invalid_argument("attack defense replacement must target an attack resource: " + replacement.targetResourceName_);
        }
        const std::string id = replacement.id_;
        if (!defenseReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("attack defense replacement is already registered: " + id);
        }
    }

    void Attacks::removeDefenseReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (defenseReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("attack defense replacement is not registered: " + id);
        }
    }

    std::vector<std::string> Attacks::attackParentResources(const AttackGrant &grant) const
    {
        std::vector<std::string> parents{attackResourceName(grant.mode_)};
        parents.reserve(grant.tags_.size() + 1);
        for (AttackTag tag : grant.tags_)
        {
            parents.push_back(attackResourceName(tag));
        }
        return parents;
    }

    std::vector<std::string> Attacks::damageParentResources(const AttackGrant &grant) const
    {
        std::vector<std::string> parents{damageResourceName(grant.mode_)};
        parents.reserve(grant.tags_.size() + 1);
        for (AttackTag tag : grant.tags_)
        {
            parents.push_back(damageResourceName(tag));
        }
        return parents;
    }

    std::vector<std::string> Attacks::criticalConfirmationParentResources(const AttackGrant &grant) const
    {
        std::vector<std::string> parents{attackResourceName(grant.id_), criticalConfirmationResourceName(grant.mode_)};
        parents.reserve(grant.tags_.size() + 2);
        for (AttackTag tag : grant.tags_)
        {
            parents.push_back(criticalConfirmationResourceName(tag));
        }
        return parents;
    }

    std::vector<std::string> Attacks::distanceParentResources(AttackDistanceProperty property, const AttackGrant &grant) const
    {
        std::vector<std::string> parents{attackDistanceResourceName(property, grant.mode_)};
        parents.reserve(grant.tags_.size() + 1);
        for (AttackTag tag : grant.tags_)
        {
            parents.push_back(attackDistanceResourceName(property, tag));
        }
        return parents;
    }
}
