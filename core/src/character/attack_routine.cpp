#include "golarion/character/attack_routine.hpp"

#include "golarion/character/action.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/attack_routines_view.hpp"
#include "golarion/view/strikes_view.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    template<typename Value>
    bool contains(const std::vector<Value> &values, const Value &value)
    {
        return std::ranges::find(values, value) != values.end();
    }

    template<typename Value>
    bool containsDuplicates(std::vector<Value> values)
    {
        std::ranges::sort(values);
        return std::ranges::adjacent_find(values) != values.end();
    }

    int checkedRoutineValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("routine attack bonus adjustment is out of range");
        }
        return static_cast<int>(value);
    }

    bool isWeapon(const golarion::StrikeView &strike)
    {
        return contains(strike.tags, golarion::AttackTag::Weapon);
    }

    bool acceptsHandUsage(golarion::WeaponWeight weaponWeight, golarion::HandUsage handUsage)
    {
        return handUsage == golarion::HandUsage::TwoHanded || weaponWeight != golarion::WeaponWeight::TwoHanded;
    }

    golarion::DamageAbilityRule effectiveDamageAbilityRule(const golarion::StrikeView &strike, golarion::StrikeUsage strikeUsage, const std::optional<golarion::HandUsage> &handUsage, const std::optional<golarion::AttackHandRole> &handRole, const std::optional<golarion::DamageAbilityRule> &override)
    {
        if (override.has_value())
        {
            return *override;
        }
        if (!isWeapon(strike))
        {
            if (strikeUsage == golarion::StrikeUsage::NaturalSecondary)
            {
                return golarion::DamageAbilityRule::HalfPositiveFullPenalty;
            }
            if (strikeUsage == golarion::StrikeUsage::SingleNatural)
            {
                return golarion::DamageAbilityRule::OneAndHalfPositiveFullPenalty;
            }
            return strike.damageAbilityRule;
        }
        if (!handUsage.has_value() || !handRole.has_value())
        {
            return strike.damageAbilityRule;
        }
        if (strike.damageAbilityRule == golarion::DamageAbilityRule::None || strike.damageAbilityRule == golarion::DamageAbilityRule::PenaltyOnly)
        {
            return strike.damageAbilityRule;
        }
        if (*handRole == golarion::AttackHandRole::OffHand)
        {
            return golarion::DamageAbilityRule::HalfPositiveFullPenalty;
        }
        if (*handUsage == golarion::HandUsage::TwoHanded && strike.effectiveWeaponWeight != golarion::WeaponWeight::Light)
        {
            return golarion::DamageAbilityRule::OneAndHalfPositiveFullPenalty;
        }
        return golarion::DamageAbilityRule::Full;
    }
}

namespace golarion
{
    std::string_view displayName(RoutineSlotSelectionMode mode)
    {
        switch (mode)
        {
            case RoutineSlotSelectionMode::ChooseOne:
                return "Scegli uno";
            case RoutineSlotSelectionMode::AllMatching:
                return "Tutti i compatibili";
        }

        throw std::invalid_argument("unknown routine slot selection mode");
    }

    std::string_view displayName(RoutineAttackProgressionType type)
    {
        switch (type)
        {
            case RoutineAttackProgressionType::Fixed:
                return "Fissa";
            case RoutineAttackProgressionType::BaseAttackBonusIteratives:
                return "Attacchi iterativi da BAB";
        }

        throw std::invalid_argument("unknown routine attack progression type");
    }

    std::string_view displayName(HandUsage usage)
    {
        switch (usage)
        {
            case HandUsage::OneHanded:
                return "Una mano";
            case HandUsage::TwoHanded:
                return "Due mani";
        }

        throw std::invalid_argument("unknown hand usage");
    }

    std::string_view displayName(AttackHandRole role)
    {
        switch (role)
        {
            case AttackHandRole::Primary:
                return "Primaria";
            case AttackHandRole::OffHand:
                return "Secondaria";
        }

        throw std::invalid_argument("unknown attack hand role");
    }

    StrikeSelector::StrikeSelector(StrikeSelectorDefinition definition)
        : allowedModes_(std::move(definition.allowedModes)),
          requiredTags_(std::move(definition.requiredTags)),
          forbiddenTags_(std::move(definition.forbiddenTags)),
          allowedGrantIds_(std::move(definition.allowedGrantIds))
    {
        if (containsDuplicates(allowedModes_))
        {
            throw std::invalid_argument("strike selector allowed modes must not contain duplicates");
        }
        if (containsDuplicates(requiredTags_))
        {
            throw std::invalid_argument("strike selector required tags must not contain duplicates");
        }
        if (containsDuplicates(forbiddenTags_))
        {
            throw std::invalid_argument("strike selector forbidden tags must not contain duplicates");
        }
        for (AttackTag tag : requiredTags_)
        {
            if (contains(forbiddenTags_, tag))
            {
                throw std::invalid_argument("strike selector cannot require and forbid the same tag");
            }
        }
        for (std::string &grantId : allowedGrantIds_)
        {
            grantId = normalize(grantId);
        }
        if (containsDuplicates(allowedGrantIds_))
        {
            throw std::invalid_argument("strike selector allowed grant IDs must not contain duplicates");
        }
    }

    RoutineAttackProgression::RoutineAttackProgression(RoutineAttackProgressionDefinition definition)
        : type_(definition.type),
          countExpression_(std::move(definition.countExpression)),
          attackBonusAdjustmentExpression_(normalize(definition.attackBonusAdjustmentExpression))
    {
        if (countExpression_.has_value())
        {
            countExpression_ = normalize(*countExpression_);
        }
        if (type_ == RoutineAttackProgressionType::Fixed && !countExpression_.has_value())
        {
            throw std::invalid_argument("fixed routine attack progression requires a count expression");
        }
        if (type_ == RoutineAttackProgressionType::BaseAttackBonusIteratives && countExpression_.has_value())
        {
            throw std::invalid_argument("BAB iterative routine attack progression must not define a count expression");
        }
    }

    RoutineSlot::RoutineSlot(RoutineSlotDefinition definition)
        : id_(normalize(definition.id)),
          name_(normalize(definition.name)),
          selector_(std::move(definition.selector)),
          selectionMode_(definition.selectionMode),
          strikeUsage_(definition.strikeUsage),
          progressions_(std::move(definition.progressions)),
          damageAbilityRuleOverride_(definition.damageAbilityRuleOverride),
          baseAttackBonusAdjustmentExpression_(normalize(definition.baseAttackBonusAdjustmentExpression)),
          handUsage_(definition.handUsage),
          handRole_(definition.handRole),
          assignmentRequired_(definition.assignmentRequired)
    {
        if (progressions_.empty())
        {
            throw std::invalid_argument("routine slot requires at least one attack progression");
        }
        const auto babProgressionCount = std::ranges::count(progressions_, RoutineAttackProgressionType::BaseAttackBonusIteratives, &RoutineAttackProgression::type_);
        if (babProgressionCount > 1)
        {
            throw std::invalid_argument("routine slot must not contain multiple BAB iterative progressions");
        }
        if (handUsage_.has_value() != handRole_.has_value())
        {
            throw std::invalid_argument("routine slot hand usage and hand role must be defined together");
        }
        if (handUsage_ == HandUsage::TwoHanded && handRole_ == AttackHandRole::OffHand)
        {
            throw std::invalid_argument("an off-hand routine slot cannot use two hands");
        }
    }

    AttackRoutine::AttackRoutine(AttackRoutineDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          name_(normalize(definition.name)),
          actionId_(normalize(definition.actionId)),
          slots_(std::move(definition.slots))
    {
        if (slots_.empty())
        {
            throw std::invalid_argument("attack routine requires at least one slot");
        }
        std::vector<std::string> slotIds;
        slotIds.reserve(slots_.size());
        for (const RoutineSlot &slot : slots_)
        {
            slotIds.push_back(slot.id_);
        }
        if (containsDuplicates(std::move(slotIds)))
        {
            throw std::invalid_argument("attack routine slot IDs must be unique");
        }
    }

    RoutineProgressionGrant::RoutineProgressionGrant(RoutineProgressionGrantDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetRoutineId_(normalize(definition.targetRoutineId)),
          targetSlotId_(normalize(definition.targetSlotId)),
          progression_(std::move(definition.progression))
    {
    }

    RoutineAssignmentRequirement::RoutineAssignmentRequirement(RoutineAssignmentRequirementDefinition definition)
        : slotId_(normalize(definition.slotId)),
          strikeGrantId_(std::move(definition.strikeGrantId)),
          requiredTag_(definition.requiredTag),
          weaponWeightPurpose_(definition.weaponWeightPurpose),
          weaponWeight_(definition.weaponWeight)
    {
        if (strikeGrantId_.has_value())
        {
            strikeGrantId_ = normalize(*strikeGrantId_);
        }
        if (weaponWeightPurpose_.has_value() != weaponWeight_.has_value())
        {
            throw std::invalid_argument("routine assignment weapon-weight requirement requires both purpose and weight");
        }
    }

    RoutineAttackBonusAdjustment::RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetRoutineId_(normalize(definition.targetRoutineId)),
          targetSlotId_(std::move(definition.targetSlotId)),
          expression_(normalize(definition.expression)),
          requirements_(std::move(definition.requirements))
    {
        if (targetSlotId_.has_value())
        {
            targetSlotId_ = normalize(*targetSlotId_);
        }
    }

    AttackRoutines::AttackRoutines(ResourceManager &resourceManager, ActionManager &actionManager, Strikes &strikes) : resourceManager_(resourceManager), actionManager_(actionManager), strikes_(strikes)
    {
        resourceManager_.registerCollectionResource<AttackRoutine>(AttackRoutinesResource, [this](AttackRoutine routine)
        {
            addRoutine(std::move(routine));
        }, [this](std::string_view routineId)
        {
            removeRoutine(routineId);
        });
        resourceManager_.registerCollectionResource<RoutineProgressionGrant>(AttackRoutineProgressionGrantsResource, [this](RoutineProgressionGrant grant)
        {
            addProgressionGrant(std::move(grant));
        }, [this](std::string_view grantId)
        {
            removeProgressionGrant(grantId);
        });
        resourceManager_.registerCollectionResource<RoutineAttackBonusAdjustment>(AttackRoutineBonusAdjustmentsResource, [this](RoutineAttackBonusAdjustment adjustment)
        {
            addAttackBonusAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeAttackBonusAdjustment(adjustmentId);
        });
        registerCanonicalRoutines();
    }

    AttackRoutinesView AttackRoutines::toView()
    {
        const StrikesView strikesView = strikes_.toView();
        std::vector<AttackRoutineView> routineViews;
        routineViews.reserve(routines_.size());

        for (const AttackRoutine &routine : routines_)
        {
            const std::optional<ActionView> action = actionManager_.actionView(routine.actionId_);
            std::vector<std::string> failureReasons;
            if (!action.has_value())
            {
                failureReasons.push_back("L'azione associata non è registrata");
            }
            else
            {
                for (const ActionInhibitionView &inhibition : action->inhibitions)
                {
                    failureReasons.push_back(inhibition.source + ": " + inhibition.reason);
                }
            }
            std::vector<RoutineSlotView> slotViews;
            slotViews.reserve(routine.slots_.size());
            for (const RoutineSlot &slot : routine.slots_)
            {
                const int baseAttackBonusAdjustment = resourceManager_.evaluateExpression(slot.baseAttackBonusAdjustmentExpression_);
                std::vector<RoutineAttackProgressionView> progressionViews;
                const auto appendProgression = [this, &progressionViews](const RoutineAttackProgression &progression, std::optional<std::string> grantId, std::optional<std::string> grantSource)
                {
                    const int baseAdjustment = resourceManager_.evaluateExpression(progression.attackBonusAdjustmentExpression_);
                    std::vector<int> attackBonusAdjustments;
                    if (progression.type_ == RoutineAttackProgressionType::Fixed)
                    {
                        const int count = resourceManager_.evaluateExpression(*progression.countExpression_);
                        if (count < 0)
                        {
                            throw std::invalid_argument("fixed routine attack progression count must not be negative");
                        }
                        attackBonusAdjustments.assign(static_cast<std::size_t>(count), baseAdjustment);
                    }
                    else
                    {
                        const int baseAttackBonus = resourceManager_.targetValue("bab");
                        const int attackCount = std::max(1, (baseAttackBonus + 4) / 5);
                        attackBonusAdjustments.reserve(static_cast<std::size_t>(attackCount));
                        for (int attackIndex = 0; attackIndex < attackCount; ++attackIndex)
                        {
                            attackBonusAdjustments.push_back(checkedRoutineValue(static_cast<long long>(baseAdjustment) - static_cast<long long>(attackIndex) * 5));
                        }
                    }
                    progressionViews.push_back(RoutineAttackProgressionView{
                        .type = progression.type_,
                        .countExpression = progression.countExpression_,
                        .attackBonusAdjustmentExpression = progression.attackBonusAdjustmentExpression_,
                        .attackBonusAdjustments = std::move(attackBonusAdjustments),
                        .grantId = std::move(grantId),
                        .grantSource = std::move(grantSource)
                    });
                };
                progressionViews.reserve(slot.progressions_.size() + progressionGrants_.size());
                for (const RoutineAttackProgression &progression : slot.progressions_)
                {
                    appendProgression(progression, std::nullopt, std::nullopt);
                }
                for (const auto &[id, grant] : progressionGrants_)
                {
                    if (grant.targetRoutineId_ == routine.id_ && grant.targetSlotId_ == slot.id_)
                    {
                        appendProgression(grant.progression_, id, grant.source_);
                    }
                }

                std::vector<RoutineAttackBonusAdjustmentView> attackBonusAdjustmentViews;
                for (const auto &[id, adjustment] : attackBonusAdjustments_)
                {
                    if (adjustment.targetRoutineId_ != routine.id_ || (adjustment.targetSlotId_.has_value() && *adjustment.targetSlotId_ != slot.id_))
                    {
                        continue;
                    }
                    std::vector<RoutineAssignmentRequirementView> requirementViews;
                    requirementViews.reserve(adjustment.requirements_.size());
                    for (const RoutineAssignmentRequirement &requirement : adjustment.requirements_)
                    {
                        requirementViews.push_back(RoutineAssignmentRequirementView{
                            .slotId = requirement.slotId_,
                            .strikeGrantId = requirement.strikeGrantId_,
                            .requiredTag = requirement.requiredTag_,
                            .weaponWeightPurpose = requirement.weaponWeightPurpose_,
                            .weaponWeight = requirement.weaponWeight_
                        });
                    }
                    attackBonusAdjustmentViews.push_back(RoutineAttackBonusAdjustmentView{
                        .id = id,
                        .source = adjustment.source_,
                        .expression = adjustment.expression_,
                        .resolvedValue = resourceManager_.evaluateExpression(adjustment.expression_),
                        .requirements = std::move(requirementViews)
                    });
                }

                std::vector<RoutineStrikeCandidateView> candidateViews;
                candidateViews.reserve(strikesView.strikes.size());
                for (const StrikeView &strike : strikesView.strikes)
                {
                    std::vector<std::string> rejectionReasons;
                    if (!slot.selector_.allowedModes_.empty() && !contains(slot.selector_.allowedModes_, strike.mode))
                    {
                        rejectionReasons.push_back("La modalità dello strike non è consentita");
                    }
                    for (AttackTag requiredTag : slot.selector_.requiredTags_)
                    {
                        if (!contains(strike.tags, requiredTag))
                        {
                            rejectionReasons.push_back("Manca il tratto richiesto: " + std::string(displayName(requiredTag)));
                        }
                    }
                    for (AttackTag forbiddenTag : slot.selector_.forbiddenTags_)
                    {
                        if (contains(strike.tags, forbiddenTag))
                        {
                            rejectionReasons.push_back("Tratto non consentito: " + std::string(displayName(forbiddenTag)));
                        }
                    }
                    if (!slot.selector_.allowedGrantIds_.empty() && !contains(slot.selector_.allowedGrantIds_, strike.grantId))
                    {
                        rejectionReasons.push_back("Il grant dello strike non è consentito");
                    }
                    if (slot.strikeUsage_ != StrikeUsage::Default && !contains(strike.tags, AttackTag::Natural))
                    {
                        rejectionReasons.push_back("L'uso richiesto è riservato agli strike naturali");
                    }
                    const bool weapon = isWeapon(strike);
                    if (!weapon && slot.handUsage_.has_value())
                    {
                        rejectionReasons.push_back("L'impugnatura dello slot è riservata agli strike con arma");
                    }
                    if (weapon && slot.handUsage_.has_value() && !acceptsHandUsage(*strike.effectiveWeaponWeight, *slot.handUsage_))
                    {
                        rejectionReasons.push_back("L'arma richiede due mani");
                    }
                    candidateViews.push_back(RoutineStrikeCandidateView{
                        .grantId = strike.grantId,
                        .name = strike.name,
                        .usageChannels = strike.usageChannels,
                        .effectiveDamageAbilityRule = effectiveDamageAbilityRule(strike, slot.strikeUsage_, slot.handUsage_, slot.handRole_, slot.damageAbilityRuleOverride_),
                        .accepted = rejectionReasons.empty(),
                        .rejectionReasons = std::move(rejectionReasons)
                    });
                }

                slotViews.push_back(RoutineSlotView{
                    .id = slot.id_,
                    .name = slot.name_,
                    .selector = StrikeSelectorView{
                        .allowedModes = slot.selector_.allowedModes_,
                        .requiredTags = slot.selector_.requiredTags_,
                        .forbiddenTags = slot.selector_.forbiddenTags_,
                        .allowedGrantIds = slot.selector_.allowedGrantIds_
                    },
                    .selectionMode = slot.selectionMode_,
                    .strikeUsage = slot.strikeUsage_,
                    .baseAttackBonusAdjustmentExpression = slot.baseAttackBonusAdjustmentExpression_,
                    .baseAttackBonusAdjustment = baseAttackBonusAdjustment,
                    .progressions = std::move(progressionViews),
                    .attackBonusAdjustments = std::move(attackBonusAdjustmentViews),
                    .damageAbilityRuleOverride = slot.damageAbilityRuleOverride_,
                    .handUsage = slot.handUsage_,
                    .handRole = slot.handRole_,
                    .assignmentRequired = slot.assignmentRequired_,
                    .candidates = std::move(candidateViews)
                });
            }
            routineViews.push_back(AttackRoutineView{
                .id = routine.id_,
                .source = routine.source_,
                .name = routine.name_,
                .actionId = routine.actionId_,
                .actionName = action.has_value() ? std::optional(action->name) : std::nullopt,
                .usable = action.has_value() && action->usable,
                .failureReasons = std::move(failureReasons),
                .slots = std::move(slotViews)
            });
        }

        return AttackRoutinesView{.routines = std::move(routineViews)};
    }

    void AttackRoutines::addRoutine(AttackRoutine routine)
    {
        if (std::ranges::find(routines_, routine.id_, &AttackRoutine::id_) != routines_.end())
        {
            throw std::invalid_argument("attack routine is already registered: " + routine.id_);
        }
        routines_.push_back(std::move(routine));
    }

    void AttackRoutines::removeRoutine(std::string_view routineId)
    {
        const std::string id = normalize(routineId);
        const auto routine = std::ranges::find(routines_, id, &AttackRoutine::id_);
        if (routine == routines_.end())
        {
            throw std::invalid_argument("attack routine is not registered: " + id);
        }
        const auto progressionGrant = std::ranges::find_if(progressionGrants_, [&id](const auto &entry)
        {
            return entry.second.targetRoutineId_ == id;
        });
        if (progressionGrant != progressionGrants_.end())
        {
            throw std::invalid_argument("attack routine is targeted by progression grant: " + progressionGrant->first);
        }
        const auto adjustment = std::ranges::find_if(attackBonusAdjustments_, [&id](const auto &entry)
        {
            return entry.second.targetRoutineId_ == id;
        });
        if (adjustment != attackBonusAdjustments_.end())
        {
            throw std::invalid_argument("attack routine is targeted by attack bonus adjustment: " + adjustment->first);
        }
        routines_.erase(routine);
    }

    void AttackRoutines::addProgressionGrant(RoutineProgressionGrant grant)
    {
        const auto routine = std::ranges::find(routines_, grant.targetRoutineId_, &AttackRoutine::id_);
        if (routine == routines_.end())
        {
            throw std::invalid_argument("routine progression grant targets an unregistered routine: " + grant.targetRoutineId_);
        }
        if (std::ranges::find(routine->slots_, grant.targetSlotId_, &RoutineSlot::id_) == routine->slots_.end())
        {
            throw std::invalid_argument("routine progression grant targets an unregistered slot: " + grant.targetSlotId_);
        }
        const std::string id = grant.id_;
        if (!progressionGrants_.emplace(id, std::move(grant)).second)
        {
            throw std::invalid_argument("routine progression grant is already registered: " + id);
        }
    }

    void AttackRoutines::removeProgressionGrant(std::string_view grantId)
    {
        const std::string id = normalize(grantId);
        if (progressionGrants_.erase(id) == 0)
        {
            throw std::invalid_argument("routine progression grant is not registered: " + id);
        }
    }

    void AttackRoutines::addAttackBonusAdjustment(RoutineAttackBonusAdjustment adjustment)
    {
        const auto routine = std::ranges::find(routines_, adjustment.targetRoutineId_, &AttackRoutine::id_);
        if (routine == routines_.end())
        {
            throw std::invalid_argument("routine attack bonus adjustment targets an unregistered routine: " + adjustment.targetRoutineId_);
        }
        const auto hasSlot = [&routine](std::string_view slotId)
        {
            return std::ranges::find(routine->slots_, slotId, &RoutineSlot::id_) != routine->slots_.end();
        };
        if (adjustment.targetSlotId_.has_value() && !hasSlot(*adjustment.targetSlotId_))
        {
            throw std::invalid_argument("routine attack bonus adjustment targets an unregistered slot: " + *adjustment.targetSlotId_);
        }
        for (const RoutineAssignmentRequirement &requirement : adjustment.requirements_)
        {
            if (!hasSlot(requirement.slotId_))
            {
                throw std::invalid_argument("routine attack bonus adjustment requires an unregistered slot: " + requirement.slotId_);
            }
        }
        const std::string id = adjustment.id_;
        if (!attackBonusAdjustments_.emplace(id, std::move(adjustment)).second)
        {
            throw std::invalid_argument("routine attack bonus adjustment is already registered: " + id);
        }
    }

    void AttackRoutines::removeAttackBonusAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (attackBonusAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("routine attack bonus adjustment is not registered: " + id);
        }
    }
}
