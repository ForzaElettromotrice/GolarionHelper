#include "golarion/character/movement.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/movement_view.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace
{
    std::string_view resourceSegment(golarion::MovementType type)
    {
        switch (type)
        {
            case golarion::MovementType::Land:
                return "land";
            case golarion::MovementType::Climb:
                return "climb";
            case golarion::MovementType::Swim:
                return "swim";
            case golarion::MovementType::Burrow:
                return "burrow";
            case golarion::MovementType::Fly:
                return "fly";
        }

        throw std::invalid_argument("unknown movement type");
    }

    int checkedSpeed(long long value)
    {
        if (value < 0 || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("movement speed is out of range");
        }
        return static_cast<int>(value);
    }

    int checkedRunValue(long long value)
    {
        if (value < 0 || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("run value is out of range");
        }
        return static_cast<int>(value);
    }

    int reducedSpeed(int speed)
    {
        return speed - speed / 3;
    }

    golarion::Maneuverability shiftedManeuverability(golarion::Maneuverability maneuverability, int shift)
    {
        const int value = std::clamp(static_cast<int>(maneuverability) + shift, static_cast<int>(golarion::Maneuverability::Clumsy), static_cast<int>(golarion::Maneuverability::Perfect));
        return static_cast<golarion::Maneuverability>(value);
    }
}

namespace golarion
{
    std::string_view displayName(MovementType type)
    {
        switch (type)
        {
            case MovementType::Land:
                return "Terreno";
            case MovementType::Climb:
                return "Scalare";
            case MovementType::Swim:
                return "Nuotare";
            case MovementType::Burrow:
                return "Scavare";
            case MovementType::Fly:
                return "Volare";
        }

        throw std::invalid_argument("unknown movement type");
    }

    std::string_view displayName(Maneuverability maneuverability)
    {
        switch (maneuverability)
        {
            case Maneuverability::Clumsy:
                return "Maldestra";
            case Maneuverability::Poor:
                return "Scarsa";
            case Maneuverability::Average:
                return "Media";
            case Maneuverability::Good:
                return "Buona";
            case Maneuverability::Perfect:
                return "Perfetta";
        }

        throw std::invalid_argument("unknown maneuverability");
    }

    std::string_view displayName(MovementAdjustmentType type)
    {
        switch (type)
        {
            case MovementAdjustmentType::SpeedMultiplier:
                return "Moltiplicatore di velocità";
            case MovementAdjustmentType::ReducedByArmorOrLoad:
                return "Velocità ridotta da armatura o carico";
            case MovementAdjustmentType::SpeedLimit:
                return "Limite di velocità";
            case MovementAdjustmentType::ManeuverabilityChange:
                return "Modifica alla manovrabilità";
            case MovementAdjustmentType::Block:
                return "Blocco del movimento";
        }

        throw std::invalid_argument("unknown movement adjustment type");
    }

    std::string_view displayName(RunAdjustmentType type)
    {
        switch (type)
        {
            case RunAdjustmentType::Increase:
                return "Incremento del moltiplicatore di corsa";
            case RunAdjustmentType::Penalty:
                return "Riduzione del moltiplicatore di corsa";
            case RunAdjustmentType::Replacement:
                return "Sostituzione del moltiplicatore di corsa";
            case RunAdjustmentType::Block:
                return "Blocco della corsa";
        }

        throw std::invalid_argument("unknown run adjustment type");
    }

    int flyCheckModifier(Maneuverability maneuverability)
    {
        switch (maneuverability)
        {
            case Maneuverability::Clumsy:
                return -8;
            case Maneuverability::Poor:
                return -4;
            case Maneuverability::Average:
                return 0;
            case Maneuverability::Good:
                return 4;
            case Maneuverability::Perfect:
                return 8;
        }

        throw std::invalid_argument("unknown maneuverability");
    }

    std::string movementResourceName(MovementType type)
    {
        return "speed." + std::string(resourceSegment(type));
    }

    std::string movementResourceName(MovementType type, std::string_view grantId)
    {
        return movementResourceName(type) + "." + normalize(grantId);
    }

    MovementGrant::MovementGrant(MovementGrantDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          type_(definition.type),
          baseSpeedExpression_(normalize(definition.baseSpeedExpression)),
          maneuverability_(definition.maneuverability),
          affectedByArmor_(definition.affectedByArmor),
          affectedByLoad_(definition.affectedByLoad),
          supportsRunning_(definition.supportsRunning)
    {
        if (type_ == MovementType::Fly && !maneuverability_.has_value())
        {
            maneuverability_ = Maneuverability::Average;
        }
        if (type_ != MovementType::Fly && maneuverability_.has_value())
        {
            throw std::invalid_argument("maneuverability is only valid for fly movement grants");
        }
    }

    MovementAdjustment::MovementAdjustment(MovementAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          description_(normalize(definition.description)),
          type_(definition.type),
          selector_(std::move(definition.selector)),
          expression_(std::move(definition.expression)),
          condition_(std::move(definition.condition))
    {
        if (selector_.grantId.has_value())
        {
            selector_.grantId = normalize(*selector_.grantId);
        }
        if (expression_.has_value())
        {
            expression_ = normalize(*expression_);
        }
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }

        const bool requiresExpression = type_ != MovementAdjustmentType::Block && type_ != MovementAdjustmentType::ReducedByArmorOrLoad;
        if (!requiresExpression && expression_.has_value())
        {
            throw std::invalid_argument("movement adjustment type must not have an expression");
        }
        if (requiresExpression && !expression_.has_value())
        {
            throw std::invalid_argument("movement adjustment expression is required");
        }
        if (type_ == MovementAdjustmentType::ManeuverabilityChange && selector_.type.has_value() && *selector_.type != MovementType::Fly)
        {
            throw std::invalid_argument("maneuverability adjustments can only target fly movement");
        }
    }

    RunAdjustment::RunAdjustment(RunAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          description_(normalize(definition.description)),
          type_(definition.type),
          stackingGroup_(normalize(definition.stackingGroup)),
          selector_(std::move(definition.selector)),
          expression_(std::move(definition.expression)),
          condition_(std::move(definition.condition))
    {
        if (selector_.grantId.has_value())
        {
            selector_.grantId = normalize(*selector_.grantId);
        }
        if (expression_.has_value())
        {
            expression_ = normalize(*expression_);
        }
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }

        if (type_ == RunAdjustmentType::Block && expression_.has_value())
        {
            throw std::invalid_argument("run block adjustments must not have an expression");
        }
        if (type_ != RunAdjustmentType::Block && !expression_.has_value())
        {
            throw std::invalid_argument("run adjustment expression is required");
        }
    }

    Movement::Movement(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource("speed.all");
        for (MovementType type : {MovementType::Land, MovementType::Climb, MovementType::Swim, MovementType::Burrow, MovementType::Fly})
        {
            resourceManager_.registerEnhanceableResource(movementResourceName(type), {"speed.all"});
        }
        resourceManager_.registerCollectionResource<MovementGrant>(MovementGrantsResource, [this](MovementGrant grant)
        {
            addGrant(std::move(grant));
        }, [this](std::string_view grantId)
        {
            removeGrant(grantId);
        });
        resourceManager_.registerCollectionResource<MovementAdjustment>(MovementAdjustmentsResource, [this](MovementAdjustment adjustment)
        {
            addAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<RunAdjustment>(RunAdjustmentsResource, [this](RunAdjustment adjustment)
        {
            addRunAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeRunAdjustment(adjustmentId);
        });
    }

    void Movement::addGrant(MovementGrant grant)
    {
        const auto duplicate = std::ranges::find(grants_, grant.id_, &MovementGrant::id_);
        if (duplicate != grants_.end())
        {
            throw std::invalid_argument("movement grant is already registered: " + grant.id_);
        }

        const std::string resourceName = movementResourceName(grant.type_, grant.id_);
        resourceManager_.registerEnhanceableResource(resourceName, {movementResourceName(grant.type_)});
        grants_.push_back(std::move(grant));
    }

    void Movement::removeGrant(std::string_view grantId)
    {
        const std::string normalizedId = normalize(grantId);
        const auto grant = std::ranges::find(grants_, normalizedId, &MovementGrant::id_);
        if (grant == grants_.end())
        {
            throw std::invalid_argument("movement grant is not registered: " + normalizedId);
        }

        const auto dependentAdjustment = std::ranges::find_if(adjustments_, [&normalizedId](const MovementAdjustment &adjustment)
        {
            return adjustment.selector_.grantId == normalizedId;
        });
        if (dependentAdjustment != adjustments_.end())
        {
            throw std::invalid_argument("movement grant is targeted by adjustment: " + dependentAdjustment->id_);
        }
        const auto dependentRunAdjustment = std::ranges::find_if(runAdjustments_, [&normalizedId](const auto &entry)
        {
            return entry.second.selector_.grantId == normalizedId;
        });
        if (dependentRunAdjustment != runAdjustments_.end())
        {
            throw std::invalid_argument("movement grant is targeted by run adjustment: " + dependentRunAdjustment->first);
        }

        resourceManager_.unregisterEnhanceableResource(movementResourceName(grant->type_, grant->id_));
        grants_.erase(grant);
    }

    void Movement::addAdjustment(MovementAdjustment adjustment)
    {
        const auto duplicate = std::ranges::find(adjustments_, adjustment.id_, &MovementAdjustment::id_);
        if (duplicate != adjustments_.end())
        {
            throw std::invalid_argument("movement adjustment is already registered: " + adjustment.id_);
        }

        if (adjustment.selector_.grantId.has_value())
        {
            const auto grant = std::ranges::find(grants_, *adjustment.selector_.grantId, &MovementGrant::id_);
            if (grant == grants_.end())
            {
                throw std::invalid_argument("movement adjustment grant is not registered: " + *adjustment.selector_.grantId);
            }
            if (adjustment.selector_.type.has_value() && *adjustment.selector_.type != grant->type_)
            {
                throw std::invalid_argument("movement adjustment selector type does not match its grant");
            }
            if (adjustment.type_ == MovementAdjustmentType::ManeuverabilityChange && grant->type_ != MovementType::Fly)
            {
                throw std::invalid_argument("maneuverability adjustments can only target fly movement");
            }
        }

        adjustments_.push_back(std::move(adjustment));
    }

    void Movement::removeAdjustment(std::string_view adjustmentId)
    {
        const std::string normalizedId = normalize(adjustmentId);
        const auto adjustment = std::ranges::find(adjustments_, normalizedId, &MovementAdjustment::id_);
        if (adjustment == adjustments_.end())
        {
            throw std::invalid_argument("movement adjustment is not registered: " + normalizedId);
        }
        adjustments_.erase(adjustment);
    }

    void Movement::addRunAdjustment(RunAdjustment adjustment)
    {
        if (adjustment.selector_.grantId.has_value())
        {
            const auto grant = std::ranges::find(grants_, *adjustment.selector_.grantId, &MovementGrant::id_);
            if (grant == grants_.end())
            {
                throw std::invalid_argument("run adjustment grant is not registered: " + *adjustment.selector_.grantId);
            }
            if (adjustment.selector_.type.has_value() && *adjustment.selector_.type != grant->type_)
            {
                throw std::invalid_argument("run adjustment selector type does not match its grant");
            }
            if (!grant->supportsRunning_)
            {
                throw std::invalid_argument("run adjustment grant does not support running: " + grant->id_);
            }
        }

        const std::string id = adjustment.id_;
        if (!runAdjustments_.emplace(id, std::move(adjustment)).second)
        {
            throw std::invalid_argument("run adjustment is already registered: " + id);
        }
    }

    void Movement::removeRunAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (runAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("run adjustment is not registered: " + id);
        }
    }

    MovementView Movement::toView()
    {
        std::vector<MovementGrantView> grantViews;
        grantViews.reserve(grants_.size());

        for (const MovementGrant &grant : grants_)
        {
            const std::string resourceName = movementResourceName(grant.type_, grant.id_);
            ModifierSetView modifiers = resourceManager_.modifierSetView(resourceName);
            const int baseUnits = resourceManager_.evaluateExpression(grant.baseSpeedExpression_);
            if (baseUnits < 0)
            {
                throw std::invalid_argument("movement grant expression must not resolve to a negative value: " + grant.baseSpeedExpression_);
            }

            int effectiveUnits = checkedSpeed(std::max(0LL, static_cast<long long>(baseUnits) + modifiers.total));
            const int modifiedBaseUnits = effectiveUnits;
            std::optional<Maneuverability> maneuverability = grant.maneuverability_;
            bool usable = true;
            bool reducedByArmorOrLoad = false;
            std::vector<int> speedLimits;
            std::vector<MovementAdjustmentView> adjustmentViews;

            for (const MovementAdjustment &adjustment : adjustments_)
            {
                if (!adjustmentMatches(adjustment, grant))
                {
                    continue;
                }

                std::optional<int> resolvedValue;
                if (adjustment.expression_.has_value())
                {
                    resolvedValue = resourceManager_.evaluateExpression(*adjustment.expression_);
                }
                adjustmentViews.push_back(MovementAdjustmentView{
                    .id = adjustment.id_,
                    .source = adjustment.source_,
                    .description = adjustment.description_,
                    .type = adjustment.type_,
                    .expression = adjustment.expression_,
                    .condition = adjustment.condition_,
                    .resolvedValue = resolvedValue
                });

                if (adjustment.condition_.has_value())
                {
                    continue;
                }

                switch (adjustment.type_)
                {
                    case MovementAdjustmentType::SpeedMultiplier:
                        if (*resolvedValue < 0)
                        {
                            throw std::invalid_argument("movement speed multiplier must not be negative");
                        }
                        effectiveUnits = checkedSpeed(static_cast<long long>(effectiveUnits) * *resolvedValue / 100);
                        break;
                    case MovementAdjustmentType::ReducedByArmorOrLoad:
                        reducedByArmorOrLoad = true;
                        break;
                    case MovementAdjustmentType::SpeedLimit:
                        if (*resolvedValue < 0)
                        {
                            throw std::invalid_argument("movement speed limit must not be negative");
                        }
                        speedLimits.push_back(*resolvedValue);
                        break;
                    case MovementAdjustmentType::ManeuverabilityChange:
                        if (maneuverability.has_value())
                        {
                            maneuverability = shiftedManeuverability(*maneuverability, *resolvedValue);
                        }
                        break;
                    case MovementAdjustmentType::Block:
                        usable = false;
                        break;
                }
            }

            if (reducedByArmorOrLoad)
            {
                effectiveUnits = reducedSpeed(effectiveUnits);
            }
            for (const int speedLimit : speedLimits)
            {
                effectiveUnits = std::min(effectiveUnits, speedLimit);
            }

            RunView runView{
                .supported = grant.supportsRunning_,
                .usable = false,
                .baseMultiplier = 4,
                .effectiveMultiplier = std::nullopt,
                .distanceUnits = std::nullopt,
                .notUsableReasons = {},
                .adjustments = {}
            };
            if (!grant.supportsRunning_)
            {
                runView.notUsableReasons.push_back("Questa modalità di movimento non supporta la corsa");
            }
            else
            {
                struct ResolvedRunAdjustment
                {
                    const RunAdjustment *adjustment;
                    std::optional<int> value;
                };

                std::vector<ResolvedRunAdjustment> resolvedRunAdjustments;
                for (const auto &[id, adjustment] : runAdjustments_)
                {
                    static_cast<void>(id);
                    if (!runAdjustmentMatches(adjustment, grant))
                    {
                        continue;
                    }

                    std::optional<int> value;
                    if (adjustment.expression_.has_value())
                    {
                        value = resourceManager_.evaluateExpression(*adjustment.expression_);
                        if (*value < 0)
                        {
                            throw std::invalid_argument("run adjustment expression must not resolve to a negative value: " + *adjustment.expression_);
                        }
                        if (adjustment.type_ == RunAdjustmentType::Replacement && *value == 0)
                        {
                            throw std::invalid_argument("run multiplier replacement must resolve to a value greater than 0");
                        }
                    }
                    resolvedRunAdjustments.push_back(ResolvedRunAdjustment{
                        .adjustment = &adjustment,
                        .value = value
                    });
                }

                using RunAdjustmentGroup = std::pair<RunAdjustmentType, std::string>;
                std::map<RunAdjustmentGroup, std::size_t> selectedByGroup;
                for (std::size_t index = 0; index < resolvedRunAdjustments.size(); ++index)
                {
                    const ResolvedRunAdjustment &resolved = resolvedRunAdjustments[index];
                    if (resolved.adjustment->condition_.has_value())
                    {
                        continue;
                    }

                    const RunAdjustmentGroup group{resolved.adjustment->type_, resolved.adjustment->stackingGroup_};
                    auto [selected, inserted] = selectedByGroup.emplace(group, index);
                    if (!inserted && resolved.value.has_value() && *resolved.value > *resolvedRunAdjustments[selected->second].value)
                    {
                        selected->second = index;
                    }
                }

                int increases = 0;
                int penalties = 0;
                std::optional<int> replacement;
                bool blocked = false;
                for (const auto &[group, selectedIndex] : selectedByGroup)
                {
                    static_cast<void>(group);
                    const ResolvedRunAdjustment &resolved = resolvedRunAdjustments[selectedIndex];
                    switch (resolved.adjustment->type_)
                    {
                        case RunAdjustmentType::Increase:
                            increases = checkedRunValue(static_cast<long long>(increases) + *resolved.value);
                            break;
                        case RunAdjustmentType::Penalty:
                            penalties = checkedRunValue(static_cast<long long>(penalties) + *resolved.value);
                            break;
                        case RunAdjustmentType::Replacement:
                            if (replacement.has_value() && *replacement != *resolved.value)
                            {
                                throw std::invalid_argument("conflicting run multiplier replacements");
                            }
                            replacement = *resolved.value;
                            break;
                        case RunAdjustmentType::Block:
                            blocked = true;
                            break;
                    }
                }

                const int startingMultiplier = replacement.value_or(runView.baseMultiplier);
                const int effectiveMultiplier = checkedRunValue(std::max(0LL, static_cast<long long>(startingMultiplier) + increases - penalties));
                runView.effectiveMultiplier = effectiveMultiplier;
                runView.adjustments.reserve(resolvedRunAdjustments.size());
                for (std::size_t index = 0; index < resolvedRunAdjustments.size(); ++index)
                {
                    const ResolvedRunAdjustment &resolved = resolvedRunAdjustments[index];
                    bool applied = false;
                    std::optional<std::string> notAppliedReason;
                    if (resolved.adjustment->condition_.has_value())
                    {
                        notAppliedReason = "Effetto condizionale";
                    }
                    else
                    {
                        const RunAdjustmentGroup group{resolved.adjustment->type_, resolved.adjustment->stackingGroup_};
                        const std::size_t selectedIndex = selectedByGroup.at(group);
                        applied = selectedIndex == index;
                        if (!applied)
                        {
                            const ResolvedRunAdjustment &selected = resolvedRunAdjustments[selectedIndex];
                            if (!resolved.value.has_value() || resolved.value == selected.value)
                            {
                                notAppliedReason = "Un'altra istanza dello stesso effetto è già applicata";
                            }
                            else
                            {
                                notAppliedReason = "Superato da un aggiustamento più alto dello stesso effetto";
                            }
                        }
                    }

                    runView.adjustments.push_back(RunAdjustmentView{
                        .id = resolved.adjustment->id_,
                        .source = resolved.adjustment->source_,
                        .description = resolved.adjustment->description_,
                        .type = resolved.adjustment->type_,
                        .stackingGroup = resolved.adjustment->stackingGroup_,
                        .expression = resolved.adjustment->expression_,
                        .condition = resolved.adjustment->condition_,
                        .resolvedValue = resolved.value,
                        .applied = applied,
                        .notAppliedReason = std::move(notAppliedReason)
                    });
                    if (applied && resolved.adjustment->type_ == RunAdjustmentType::Block)
                    {
                        runView.notUsableReasons.push_back(resolved.adjustment->description_);
                    }
                }

                if (!usable)
                {
                    runView.notUsableReasons.push_back("La modalità di movimento non è utilizzabile");
                }
                if (effectiveMultiplier == 0)
                {
                    runView.notUsableReasons.push_back("Il moltiplicatore di corsa è pari a 0");
                }
                runView.usable = usable && !blocked && effectiveMultiplier > 0;
                if (runView.usable)
                {
                    runView.distanceUnits = checkedRunValue(static_cast<long long>(effectiveUnits) * effectiveMultiplier);
                }
            }

            grantViews.push_back(MovementGrantView{
                .id = grant.id_,
                .source = grant.source_,
                .type = grant.type_,
                .resourceName = resourceName,
                .baseSpeedExpression = grant.baseSpeedExpression_,
                .baseUnits = baseUnits,
                .modifiedBaseUnits = modifiedBaseUnits,
                .effectiveUnits = effectiveUnits,
                .maneuverability = maneuverability,
                .affectedByArmor = grant.affectedByArmor_,
                .affectedByLoad = grant.affectedByLoad_,
                .usable = usable,
                .run = std::move(runView),
                .modifiers = std::move(modifiers),
                .adjustments = std::move(adjustmentViews)
            });
        }

        std::ranges::sort(grantViews, [](const MovementGrantView &left, const MovementGrantView &right)
        {
            if (left.type != right.type)
            {
                return left.type < right.type;
            }
            return left.id < right.id;
        });
        return MovementView{.grants = std::move(grantViews)};
    }

    bool Movement::adjustmentMatches(const MovementAdjustment &adjustment, const MovementGrant &grant) const
    {
        if (adjustment.type_ == MovementAdjustmentType::ManeuverabilityChange && grant.type_ != MovementType::Fly)
        {
            return false;
        }
        if (adjustment.selector_.type.has_value() && *adjustment.selector_.type != grant.type_)
        {
            return false;
        }
        if (adjustment.selector_.affectedByArmorOnly && !grant.affectedByArmor_)
        {
            return false;
        }
        if (adjustment.selector_.affectedByLoadOnly && !grant.affectedByLoad_)
        {
            return false;
        }
        return !adjustment.selector_.grantId.has_value() || *adjustment.selector_.grantId == grant.id_;
    }

    bool Movement::runAdjustmentMatches(const RunAdjustment &adjustment, const MovementGrant &grant) const
    {
        if (!grant.supportsRunning_)
        {
            return false;
        }
        if (adjustment.selector_.type.has_value() && *adjustment.selector_.type != grant.type_)
        {
            return false;
        }
        if (adjustment.selector_.affectedByArmorOnly && !grant.affectedByArmor_)
        {
            return false;
        }
        if (adjustment.selector_.affectedByLoadOnly && !grant.affectedByLoad_)
        {
            return false;
        }
        return !adjustment.selector_.grantId.has_value() || *adjustment.selector_.grantId == grant.id_;
    }

}
