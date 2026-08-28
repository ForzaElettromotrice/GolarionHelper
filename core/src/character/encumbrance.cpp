#include "golarion/character/encumbrance.hpp"

#include "golarion/character/action.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/encumbrance_view.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view LoadSource = "Ingombro del peso";
    constexpr std::string_view LoadMaximumDexterityId = "encumbrance.load.maximumDexterity";
    constexpr std::string_view LoadArmorClassSuppressionId = "encumbrance.load.armorClassSuppression";
    constexpr std::string_view LoadArmorCheckPenaltyId = "encumbrance.load.armorCheckPenalty";
    constexpr std::string_view LoadMovementAdjustmentId = "encumbrance.load.movement";
    constexpr std::string_view LoadRunAdjustmentId = "encumbrance.load.run";
    constexpr std::string_view LoadRunActionInhibitionId = "encumbrance.load.runActionInhibition";
    constexpr std::string_view LoadChargeActionInhibitionId = "encumbrance.load.chargeActionInhibition";
    constexpr std::string_view LoadMoveCostReplacementId = "encumbrance.load.moveCostReplacement";

    struct LoadDefinition
    {
        golarion::LoadCategory category;
        std::optional<int> maximumDexterityBonus;
        int armorCheckPenalty;
        bool losesDexterityBonusToArmorClass;
        bool reducesMovement;
        std::optional<int> movementSpeedLimitUnits;
        int runMultiplierPenalty;
        bool preventsRunning;
    };

    const LoadDefinition &definition(golarion::LoadCategory category)
    {
        static const LoadDefinition Light{
            .category = golarion::LoadCategory::Light,
            .maximumDexterityBonus = std::nullopt,
            .armorCheckPenalty = 0,
            .losesDexterityBonusToArmorClass = false,
            .reducesMovement = false,
            .movementSpeedLimitUnits = std::nullopt,
            .runMultiplierPenalty = 0,
            .preventsRunning = false
        };
        static const LoadDefinition Medium{
            .category = golarion::LoadCategory::Medium,
            .maximumDexterityBonus = 3,
            .armorCheckPenalty = 3,
            .losesDexterityBonusToArmorClass = false,
            .reducesMovement = true,
            .movementSpeedLimitUnits = std::nullopt,
            .runMultiplierPenalty = 0,
            .preventsRunning = false
        };
        static const LoadDefinition Heavy{
            .category = golarion::LoadCategory::Heavy,
            .maximumDexterityBonus = 1,
            .armorCheckPenalty = 6,
            .losesDexterityBonusToArmorClass = false,
            .reducesMovement = true,
            .movementSpeedLimitUnits = std::nullopt,
            .runMultiplierPenalty = 1,
            .preventsRunning = false
        };
        static const LoadDefinition Overloaded{
            .category = golarion::LoadCategory::Overloaded,
            .maximumDexterityBonus = std::nullopt,
            .armorCheckPenalty = 6,
            .losesDexterityBonusToArmorClass = true,
            .reducesMovement = false,
            .movementSpeedLimitUnits = 1,
            .runMultiplierPenalty = 0,
            .preventsRunning = true
        };

        switch (category)
        {
            case golarion::LoadCategory::Light:
                return Light;
            case golarion::LoadCategory::Medium:
                return Medium;
            case golarion::LoadCategory::Heavy:
                return Heavy;
            case golarion::LoadCategory::Overloaded:
                return Overloaded;
        }
        throw std::invalid_argument("unknown load category");
    }

    std::int64_t checkedWeightSum(std::int64_t left, std::int64_t right)
    {
        if (right > std::numeric_limits<std::int64_t>::max() - left)
        {
            throw std::invalid_argument("carried weight total is out of range");
        }
        return left + right;
    }

    std::int64_t checkedWeightMultiple(std::int64_t value, std::int64_t multiplier)
    {
        if (value != 0 && multiplier > std::numeric_limits<std::int64_t>::max() / value)
        {
            return std::numeric_limits<std::int64_t>::max();
        }
        return value * multiplier;
    }
}

namespace golarion
{
    std::string_view displayName(LoadCategory category)
    {
        switch (category)
        {
            case LoadCategory::Light:
                return "Leggero";
            case LoadCategory::Medium:
                return "Medio";
            case LoadCategory::Heavy:
                return "Pesante";
            case LoadCategory::Overloaded:
                return "Sovraccarico";
        }
        throw std::invalid_argument("unknown load category");
    }

    CarriedWeight::CarriedWeight(CarriedWeightDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source)), grams_(definition.grams)
    {
        if (grams_ < 0)
        {
            throw std::invalid_argument("carried weight must not be negative");
        }
    }

    Encumbrance::Encumbrance(ResourceManager &resourceManager, CarryingCapacity &carryingCapacity)
        : resourceManager_(resourceManager), carryingCapacity_(carryingCapacity)
    {
        resourceManager_.registerCollectionResource<CarriedWeight>(CarriedWeightsResource, [this](CarriedWeight weight)
        {
            addWeight(std::move(weight));
        }, [this](std::string_view weightId)
        {
            removeWeight(weightId);
        });
    }

    EncumbranceView Encumbrance::toView()
    {
        synchronizeEffects();
        const Resolution resolution = resolve();
        const LoadDefinition &loadDefinition = definition(resolution.category);
        std::vector<CarriedWeightView> weightViews;
        weightViews.reserve(weights_.size());
        for (const auto &[id, weight] : weights_)
        {
            weightViews.push_back(CarriedWeightView{
                .id = id,
                .source = weight.source_,
                .grams = weight.grams_
            });
        }

        return EncumbranceView{
            .totalWeightGrams = resolution.totalWeightGrams,
            .weights = std::move(weightViews),
            .lightLoadMaxGrams = resolution.limits.lightGrams,
            .mediumLoadMaxGrams = resolution.limits.mediumGrams,
            .heavyLoadMaxGrams = resolution.limits.heavyGrams,
            .category = resolution.category,
            .effects = LoadEffectsView{
                .maximumDexterityBonus = loadDefinition.maximumDexterityBonus,
                .armorCheckPenalty = loadDefinition.armorCheckPenalty,
                .losesDexterityBonusToArmorClass = loadDefinition.losesDexterityBonusToArmorClass,
                .reducesMovement = loadDefinition.reducesMovement,
                .movementSpeedLimitUnits = loadDefinition.movementSpeedLimitUnits,
                .runMultiplierPenalty = loadDefinition.runMultiplierPenalty,
                .preventsRunning = loadDefinition.preventsRunning
            },
            .withinLiftFromGroundLimit = resolution.totalWeightGrams <= checkedWeightMultiple(resolution.limits.heavyGrams, 2),
            .withinPushOrDragLimit = resolution.totalWeightGrams <= checkedWeightMultiple(resolution.limits.heavyGrams, 5)
        };
    }

    Encumbrance::Resolution Encumbrance::resolve()
    {
        std::int64_t totalWeightGrams = 0;
        for (const auto &[id, weight] : weights_)
        {
            static_cast<void>(id);
            totalWeightGrams = checkedWeightSum(totalWeightGrams, weight.grams_);
        }

        const CarryingCapacity::ResolvedLoadLimits limits = carryingCapacity_.resolvedLoadLimits();
        LoadCategory category = LoadCategory::Overloaded;
        if (totalWeightGrams <= limits.lightGrams)
        {
            category = LoadCategory::Light;
        }
        else if (totalWeightGrams <= limits.mediumGrams)
        {
            category = LoadCategory::Medium;
        }
        else if (totalWeightGrams <= limits.heavyGrams)
        {
            category = LoadCategory::Heavy;
        }

        return Resolution{
            .totalWeightGrams = totalWeightGrams,
            .limits = limits,
            .category = category
        };
    }

    void Encumbrance::synchronizeEffects()
    {
        const LoadCategory category = resolve().category;
        if (appliedCategory_ == category)
        {
            return;
        }

        const std::optional<LoadCategory> previousCategory = appliedCategory_;
        clearEffects();
        try
        {
            applyEffects(category);
            appliedCategory_ = category;
        }
        catch (...)
        {
            clearEffects();
            if (previousCategory.has_value())
            {
                applyEffects(*previousCategory);
                appliedCategory_ = previousCategory;
            }
            throw;
        }
    }

    void Encumbrance::applyEffects(LoadCategory category)
    {
        const LoadDefinition &loadDefinition = definition(category);
        if (loadDefinition.armorCheckPenalty > 0)
        {
            resourceManager_.addToCollection(ArmorCheckPenaltiesResource, ArmorCheckPenalty(ArmorCheckPenaltyDefinition{
                .id = std::string(LoadArmorCheckPenaltyId),
                .source = std::string(LoadSource),
                .expression = std::to_string(loadDefinition.armorCheckPenalty)
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(ArmorCheckPenaltiesResource, LoadArmorCheckPenaltyId);
            });
        }

        if (loadDefinition.reducesMovement || loadDefinition.movementSpeedLimitUnits.has_value())
        {
            const MovementAdjustmentType type = loadDefinition.reducesMovement ? MovementAdjustmentType::ReducedByArmorOrLoad : MovementAdjustmentType::SpeedLimit;
            const std::optional<std::string> expression = loadDefinition.movementSpeedLimitUnits.has_value()
                ? std::optional<std::string>(std::to_string(*loadDefinition.movementSpeedLimitUnits))
                : std::nullopt;
            resourceManager_.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
                .id = std::string(LoadMovementAdjustmentId),
                .source = std::string(LoadSource),
                .description = loadDefinition.reducesMovement ? "Velocità ridotta dal carico" : "Può muoversi di soli 1,5 metri per round",
                .type = type,
                .selector = MovementSelector{.type = std::nullopt, .grantId = std::nullopt, .affectedByArmorOnly = false, .affectedByLoadOnly = true},
                .expression = expression,
                .condition = std::nullopt
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(MovementAdjustmentsResource, LoadMovementAdjustmentId);
            });
        }

        if (loadDefinition.runMultiplierPenalty > 0 || loadDefinition.preventsRunning)
        {
            resourceManager_.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
                .id = std::string(LoadRunAdjustmentId),
                .source = std::string(LoadSource),
                .description = loadDefinition.preventsRunning ? "Non può correre mentre è sovraccarico" : "Moltiplicatore di corsa ridotto dal carico pesante",
                .type = loadDefinition.preventsRunning ? RunAdjustmentType::Block : RunAdjustmentType::Penalty,
                .stackingGroup = loadDefinition.preventsRunning ? "overloaded" : std::string(HeavyArmorOrLoadRunPenaltyGroup),
                .selector = MovementSelector{},
                .expression = loadDefinition.preventsRunning ? std::nullopt : std::optional<std::string>(std::to_string(loadDefinition.runMultiplierPenalty)),
                .condition = std::nullopt
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(RunAdjustmentsResource, LoadRunAdjustmentId);
            });
        }

        if (category == LoadCategory::Overloaded)
        {
            resourceManager_.addToCollection(ActionInhibitionsResource, ActionInhibition(ActionInhibitionDefinition{
                .id = std::string(LoadRunActionInhibitionId),
                .source = std::string(LoadSource),
                .selector = ActionSelector(ActionSelectorDefinition{.actionId = "base.run"}),
                .reason = "Non può correre mentre è sovraccarico"
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(ActionInhibitionsResource, LoadRunActionInhibitionId);
            });
            resourceManager_.addToCollection(ActionInhibitionsResource, ActionInhibition(ActionInhibitionDefinition{
                .id = std::string(LoadChargeActionInhibitionId),
                .source = std::string(LoadSource),
                .selector = ActionSelector(ActionSelectorDefinition{.actionId = "base.charge"}),
                .reason = "Non può caricare mentre è sovraccarico"
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(ActionInhibitionsResource, LoadChargeActionInhibitionId);
            });
            resourceManager_.addToCollection(ActionCostReplacementsResource, ActionCostReplacement(ActionCostReplacementDefinition{
                .id = std::string(LoadMoveCostReplacementId),
                .source = std::string(LoadSource),
                .actionId = "base.move",
                .cost = ActionCost::FullRound
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(ActionCostReplacementsResource, LoadMoveCostReplacementId);
            });
        }

        if (loadDefinition.losesDexterityBonusToArmorClass)
        {
            resourceManager_.addToCollection(ArmorClassAbilitySuppressionsResource, ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition{
                .id = std::string(LoadArmorClassSuppressionId),
                .source = std::string(LoadSource)
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(ArmorClassAbilitySuppressionsResource, LoadArmorClassSuppressionId);
            });
        }

        if (loadDefinition.maximumDexterityBonus.has_value())
        {
            resourceManager_.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
                .id = std::string(LoadMaximumDexterityId),
                .source = std::string(LoadSource),
                .type = MaximumDexterityLimitType::Load,
                .expression = std::to_string(*loadDefinition.maximumDexterityBonus)
            }));
            effectCleanups_.push_back([this]
            {
                resourceManager_.removeFromCollection(MaximumDexterityLimitsResource, LoadMaximumDexterityId);
            });
        }
    }

    void Encumbrance::clearEffects()
    {
        while (!effectCleanups_.empty())
        {
            effectCleanups_.back()();
            effectCleanups_.pop_back();
        }
        appliedCategory_.reset();
    }

    void Encumbrance::addWeight(CarriedWeight weight)
    {
        const std::string id = weight.id_;
        if (!weights_.emplace(id, std::move(weight)).second)
        {
            throw std::invalid_argument("carried weight is already registered: " + id);
        }

        try
        {
            synchronizeEffects();
        }
        catch (...)
        {
            weights_.erase(id);
            synchronizeEffects();
            throw;
        }
    }

    void Encumbrance::removeWeight(std::string_view weightId)
    {
        const std::string id = normalize(weightId);
        auto weight = weights_.extract(id);
        if (weight.empty())
        {
            throw std::invalid_argument("carried weight is not registered: " + id);
        }

        try
        {
            synchronizeEffects();
        }
        catch (...)
        {
            weights_.insert(std::move(weight));
            synchronizeEffects();
            throw;
        }
    }

}
