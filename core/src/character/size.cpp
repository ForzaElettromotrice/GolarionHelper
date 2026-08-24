#include "golarion/character/size.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/size_view.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view SizeModifierSource = "Taglia";
    constexpr std::string_view CarryingCapacitySizeId = "size.current";

    int stepsFromMedium(golarion::SizeCategory category)
    {
        switch (category)
        {
            case golarion::SizeCategory::Fine:
                return -4;
            case golarion::SizeCategory::Diminutive:
                return -3;
            case golarion::SizeCategory::Tiny:
                return -2;
            case golarion::SizeCategory::Small:
                return -1;
            case golarion::SizeCategory::Medium:
                return 0;
            case golarion::SizeCategory::Large:
                return 1;
            case golarion::SizeCategory::Huge:
                return 2;
            case golarion::SizeCategory::Gargantuan:
                return 3;
            case golarion::SizeCategory::Colossal:
                return 4;
        }

        throw std::invalid_argument("unknown size category");
    }

    golarion::SizeCategory categoryFromSteps(int steps)
    {
        switch (std::clamp(steps, -4, 4))
        {
            case -4:
                return golarion::SizeCategory::Fine;
            case -3:
                return golarion::SizeCategory::Diminutive;
            case -2:
                return golarion::SizeCategory::Tiny;
            case -1:
                return golarion::SizeCategory::Small;
            case 0:
                return golarion::SizeCategory::Medium;
            case 1:
                return golarion::SizeCategory::Large;
            case 2:
                return golarion::SizeCategory::Huge;
            case 3:
                return golarion::SizeCategory::Gargantuan;
            case 4:
                return golarion::SizeCategory::Colossal;
        }

        throw std::logic_error("clamped size category is out of range");
    }

    int attackModifier(golarion::SizeCategory category)
    {
        switch (category)
        {
            case golarion::SizeCategory::Fine:
                return 8;
            case golarion::SizeCategory::Diminutive:
                return 4;
            case golarion::SizeCategory::Tiny:
                return 2;
            case golarion::SizeCategory::Small:
                return 1;
            case golarion::SizeCategory::Medium:
                return 0;
            case golarion::SizeCategory::Large:
                return -1;
            case golarion::SizeCategory::Huge:
                return -2;
            case golarion::SizeCategory::Gargantuan:
                return -4;
            case golarion::SizeCategory::Colossal:
                return -8;
        }

        throw std::invalid_argument("unknown size category");
    }

    long long magnitude(int value)
    {
        return value < 0 ? -static_cast<long long>(value) : static_cast<long long>(value);
    }
}

namespace golarion
{
    std::string_view displayName(SizeCategory category)
    {
        switch (category)
        {
            case SizeCategory::Fine:
                return "Piccolissima";
            case SizeCategory::Diminutive:
                return "Minuta";
            case SizeCategory::Tiny:
                return "Minuscola";
            case SizeCategory::Small:
                return "Piccola";
            case SizeCategory::Medium:
                return "Media";
            case SizeCategory::Large:
                return "Grande";
            case SizeCategory::Huge:
                return "Enorme";
            case SizeCategory::Gargantuan:
                return "Mastodontica";
            case SizeCategory::Colossal:
                return "Colossale";
        }

        throw std::invalid_argument("unknown size category");
    }

    SizeBase::SizeBase(SizeBaseDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          category_(definition.category)
    {
        static_cast<void>(displayName(category_));
    }

    SizeReplacement::SizeReplacement(SizeReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          category_(definition.category),
          acceptsAdjustments_(definition.acceptsAdjustments)
    {
        static_cast<void>(displayName(category_));
    }

    SizeAdjustment::SizeAdjustment(SizeAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          steps_(definition.steps)
    {
        if (steps_ == 0)
        {
            throw std::invalid_argument("size adjustment steps must not be zero");
        }
    }

    SizeManager::SizeManager(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerCollectionResource<SizeBase>(SizeBaseResource, [this](SizeBase base)
        {
            addBase(std::move(base));
        }, [this](std::string_view baseId)
        {
            removeBase(baseId);
        });
        resourceManager_.registerCollectionResource<SizeReplacement>(SizeReplacementsResource, [this](SizeReplacement replacement)
        {
            addReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeReplacement(replacementId);
        });
        resourceManager_.registerCollectionResource<SizeAdjustment>(SizeAdjustmentsResource, [this](SizeAdjustment adjustment)
        {
            addAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeAdjustment(adjustmentId);
        });
        refreshCarryingCapacitySize();
    }

    SizeView SizeManager::toView() const
    {
        const ResolvedSize resolved = resolve();
        std::optional<SizeBaseView> baseView;
        if (base_.has_value())
        {
            baseView = SizeBaseView{
                .id = base_->id_,
                .source = base_->source_,
                .category = base_->category_
            };
        }

        std::vector<SizeReplacementView> replacementViews;
        replacementViews.reserve(replacements_.size());
        for (const auto &[id, replacement] : replacements_)
        {
            replacementViews.push_back(SizeReplacementView{
                .id = id,
                .source = replacement.source_,
                .category = replacement.category_,
                .acceptsAdjustments = replacement.acceptsAdjustments_,
                .applied = resolved.hasReplacement && replacement.category_ == resolved.referenceCategory && replacement.acceptsAdjustments_ == resolved.acceptsAdjustments
            });
        }

        std::vector<SizeAdjustmentView> adjustmentViews;
        adjustmentViews.reserve(adjustments_.size());
        for (const auto &[id, adjustment] : adjustments_)
        {
            const bool selected = adjustment.steps_ == resolved.selectedAdjustmentSteps;
            const bool applied = resolved.acceptsAdjustments && selected;
            std::optional<std::string> notAppliedReason;
            if (!resolved.acceptsAdjustments)
            {
                notAppliedReason = "La sostituzione di taglia attiva non accetta aggiustamenti";
            }
            else if (!selected)
            {
                notAppliedReason = "Superato da un aggiustamento di taglia più forte";
            }
            adjustmentViews.push_back(SizeAdjustmentView{
                .id = id,
                .source = adjustment.source_,
                .steps = adjustment.steps_,
                .applied = applied,
                .notAppliedReason = std::move(notAppliedReason)
            });
        }

        return SizeView{
            .baseCategory = resolved.baseCategory,
            .replacementCategory = resolved.hasReplacement ? std::optional(resolved.referenceCategory) : std::nullopt,
            .acceptsAdjustments = resolved.acceptsAdjustments,
            .selectedAdjustmentSteps = resolved.selectedAdjustmentSteps,
            .appliedAdjustmentSteps = resolved.appliedAdjustmentSteps,
            .effectiveCategory = resolved.effectiveCategory,
            .armorClassModifier = attackModifier(resolved.effectiveCategory),
            .attackModifier = attackModifier(resolved.effectiveCategory),
            .combatManeuverModifier = -attackModifier(resolved.effectiveCategory),
            .stealthModifier = -4 * stepsFromMedium(resolved.effectiveCategory),
            .flyModifier = -2 * stepsFromMedium(resolved.effectiveCategory),
            .base = std::move(baseView),
            .replacements = std::move(replacementViews),
            .adjustments = std::move(adjustmentViews)
        };
    }

    void SizeManager::addBase(SizeBase base)
    {
        if (base_.has_value())
        {
            throw std::invalid_argument("size base is already registered: " + base_->id_);
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        base_ = std::move(base);
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::removeBase(std::string_view baseId)
    {
        const std::string id = normalize(baseId);
        if (!base_.has_value() || base_->id_ != id)
        {
            throw std::invalid_argument("size base is not registered: " + id);
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        base_.reset();
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::addReplacement(SizeReplacement replacement)
    {
        const std::string id = replacement.id_;
        if (replacements_.contains(id))
        {
            throw std::invalid_argument("size replacement is already registered: " + id);
        }
        if (!replacements_.empty())
        {
            const SizeReplacement &registered = replacements_.begin()->second;
            if (registered.category_ != replacement.category_ || registered.acceptsAdjustments_ != replacement.acceptsAdjustments_)
            {
                throw std::invalid_argument("size replacements specify conflicting results");
            }
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        replacements_.emplace(id, std::move(replacement));
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::removeReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        const auto replacement = replacements_.find(id);
        if (replacement == replacements_.end())
        {
            throw std::invalid_argument("size replacement is not registered: " + id);
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        replacements_.erase(replacement);
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::addAdjustment(SizeAdjustment adjustment)
    {
        const std::string id = adjustment.id_;
        if (adjustments_.contains(id))
        {
            throw std::invalid_argument("size adjustment is already registered: " + id);
        }
        const auto oppositeAdjustment = std::ranges::find_if(adjustments_, [&adjustment](const auto &entry)
        {
            return (entry.second.steps_ < 0) != (adjustment.steps_ < 0);
        });
        if (oppositeAdjustment != adjustments_.end())
        {
            throw std::invalid_argument("size adjustments in opposite directions cannot coexist");
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        adjustments_.emplace(id, std::move(adjustment));
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::removeAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        const auto adjustment = adjustments_.find(id);
        if (adjustment == adjustments_.end())
        {
            throw std::invalid_argument("size adjustment is not registered: " + id);
        }
        const SizeCategory previousCategory = resolve().effectiveCategory;
        adjustments_.erase(adjustment);
        refreshAppliedModifiersIfSizeChanged(previousCategory);
    }

    void SizeManager::refreshAppliedModifiers()
    {
        for (const AppliedModifier &appliedModifier : appliedModifiers_)
        {
            resourceManager_.removeModifier(appliedModifier.resourceName, appliedModifier.modifierId);
        }
        appliedModifiers_.clear();
        refreshCarryingCapacitySize();

        const SizeCategory category = resolve().effectiveCategory;
        const int attack = attackModifier(category);
        const int combatManeuver = -attack;
        const int steps = stepsFromMedium(category);

        addAppliedModifier(attackResourceName(AttackMode::Melee), "Modificatore di taglia ai tiri per colpire in mischia", attack);
        addAppliedModifier(attackResourceName(AttackMode::Ranged), "Modificatore di taglia ai tiri per colpire a distanza", attack);
        addAppliedModifier(ArmorClassAllResource, "Modificatore di taglia alla Classe Armatura", attack);
        addAppliedModifier(CombatManeuverBonusAllResource, "Modificatore di taglia al BMC", combatManeuver);
        addAppliedModifier(CombatManeuverDefenseAllResource, "Modificatore di taglia alla DMC", combatManeuver);
        addAppliedModifier(resourceName(SkillType::Stealth), "Modificatore di taglia a Furtività", -4 * steps);
        addAppliedModifier(resourceName(SkillType::Fly), "Modificatore di taglia a Volare", -2 * steps);
    }

    void SizeManager::refreshAppliedModifiersIfSizeChanged(SizeCategory previousCategory)
    {
        if (resolve().effectiveCategory != previousCategory)
        {
            refreshAppliedModifiers();
        }
    }

    void SizeManager::refreshCarryingCapacitySize()
    {
        if (carryingCapacitySizeRegistered_)
        {
            resourceManager_.removeFromCollection(CarryingCapacitySizeResource, CarryingCapacitySizeId);
            carryingCapacitySizeRegistered_ = false;
        }

        resourceManager_.addToCollection(CarryingCapacitySizeResource, CarryingCapacitySize(CarryingCapacitySizeDefinition{
            .id = std::string(CarryingCapacitySizeId),
            .source = std::string(SizeModifierSource),
            .category = resolve().effectiveCategory
        }));
        carryingCapacitySizeRegistered_ = true;
    }

    void SizeManager::addAppliedModifier(std::string_view resourceName, std::string description, int value)
    {
        if (value == 0)
        {
            return;
        }

        Modifier modifier(
            value > 0 ? ModifierType::Bonus : ModifierType::Penalty,
            std::string(SizeModifierSource),
            std::move(description),
            value > 0 ? std::optional(BonusType::Size) : std::nullopt,
            std::to_string(magnitude(value))
        );
        AppliedModifier appliedModifier{
            .resourceName = std::string(resourceName),
            .modifierId = modifier.id()
        };
        resourceManager_.addModifier(appliedModifier.resourceName, std::move(modifier));
        appliedModifiers_.push_back(std::move(appliedModifier));
    }

    SizeManager::ResolvedSize SizeManager::resolve() const
    {
        const SizeCategory baseCategory = base_.has_value() ? base_->category_ : SizeCategory::Medium;
        const bool hasReplacement = !replacements_.empty();
        const SizeCategory referenceCategory = hasReplacement ? replacements_.begin()->second.category_ : baseCategory;
        const bool acceptsAdjustments = !hasReplacement || replacements_.begin()->second.acceptsAdjustments_;

        int selectedAdjustmentSteps = 0;
        for (const auto &[id, adjustment] : adjustments_)
        {
            static_cast<void>(id);
            if (magnitude(adjustment.steps_) > magnitude(selectedAdjustmentSteps))
            {
                selectedAdjustmentSteps = adjustment.steps_;
            }
        }

        const int referenceSteps = stepsFromMedium(referenceCategory);
        const int requestedSteps = acceptsAdjustments ? selectedAdjustmentSteps : 0;
        const SizeCategory effectiveCategory = categoryFromSteps(static_cast<int>(std::clamp(static_cast<long long>(referenceSteps) + requestedSteps, -4LL, 4LL)));
        const int appliedAdjustmentSteps = stepsFromMedium(effectiveCategory) - referenceSteps;
        return ResolvedSize{
            .baseCategory = baseCategory,
            .referenceCategory = referenceCategory,
            .effectiveCategory = effectiveCategory,
            .hasReplacement = hasReplacement,
            .acceptsAdjustments = acceptsAdjustments,
            .selectedAdjustmentSteps = selectedAdjustmentSteps,
            .appliedAdjustmentSteps = appliedAdjustmentSteps
        };
    }
}
