#include "golarion/resource/modifier_set.hpp"

#include "golarion/view/modifier_set_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    struct ResolvedModifier
    {
        const golarion::Modifier *modifier;
        int value;
    };

    int calculateCircumstanceBonuses(const std::vector<ResolvedModifier> &bonuses)
    {
        std::unordered_map<std::string, int> highestBonusBySource;
        for (const ResolvedModifier &bonus : bonuses)
        {
            auto [entry, inserted] = highestBonusBySource.emplace(bonus.modifier->source(), bonus.value);
            if (!inserted)
            {
                entry->second = std::max(entry->second, bonus.value);
            }
        }

        int total = 0;
        for (const auto &entry : highestBonusBySource)
        {
            total += entry.second;
        }
        return total;
    }

    int calculateBonusGroup(golarion::BonusType bonusType, const std::vector<ResolvedModifier> &bonuses)
    {
        int total = 0;
        switch (golarion::stackingRule(bonusType))
        {
            case golarion::StackingRule::HighestOnly:
                for (const ResolvedModifier &bonus : bonuses)
                {
                    total = std::max(total, bonus.value);
                }
                break;
            case golarion::StackingRule::Stacks:
                for (const ResolvedModifier &bonus : bonuses)
                {
                    total += bonus.value;
                }
                break;
            case golarion::StackingRule::StacksUnlessSameSource:
                total = calculateCircumstanceBonuses(bonuses);
                break;
        }

        return bonusType == golarion::BonusType::Inherent ? std::min(total, 5) : total;
    }

    int calculateResolvedTotal(const std::vector<ResolvedModifier> &modifiers)
    {
        std::map<golarion::BonusType, std::vector<ResolvedModifier>> bonusesByType;
        int penalties = 0;

        for (const ResolvedModifier &resolvedModifier : modifiers)
        {
            const golarion::Modifier &modifier = *resolvedModifier.modifier;
            if (modifier.type() == golarion::ModifierType::Penalty)
            {
                penalties += resolvedModifier.value;
            }
            else
            {
                bonusesByType[*modifier.bonusType()].push_back(resolvedModifier);
            }
        }

        int bonuses = 0;
        for (const auto &[bonusType, bonusesOfType] : bonusesByType)
        {
            bonuses += calculateBonusGroup(bonusType, bonusesOfType);
        }
        return bonuses - penalties;
    }

    struct ConditionalGroup
    {
        std::string condition;
        std::vector<ResolvedModifier> modifiers;
    };
}

namespace golarion
{
    void ModifierSet::addModifier(Modifier modifier)
    {
        const auto duplicate = std::find_if(modifiers_.begin(), modifiers_.end(), [&modifier](const Modifier &registeredModifier)
        {
            return registeredModifier.id() == modifier.id();
        });
        if (duplicate != modifiers_.end())
        {
            throw std::invalid_argument("modifier is already registered: " + modifier.id());
        }
        modifiers_.push_back(std::move(modifier));
    }

    void ModifierSet::removeModifier(std::string_view modifierId)
    {
        const auto previousSize = modifiers_.size();
        std::erase_if(modifiers_, [modifierId](const Modifier &modifier)
        {
            return modifier.id() == modifierId;
        });

        if (modifiers_.size() == previousSize)
        {
            throw std::invalid_argument("modifier is not registered: " + std::string(modifierId));
        }
    }

    bool ModifierSet::empty() const
    {
        return modifiers_.empty();
    }

    int ModifierSet::calculateTotal(ResourceManager &resourceManager) const
    {
        return calculateTotal(std::vector<const ModifierSet *>{this}, resourceManager);
    }

    int ModifierSet::calculateTotal(const std::vector<const ModifierSet *> &modifierSets, ResourceManager &resourceManager)
    {
        std::vector<ResolvedModifier> permanentModifiers;
        for (const ModifierSet *modifierSet : modifierSets)
        {
            for (const Modifier &modifier : modifierSet->modifiers_)
            {
                if (!modifier.condition().has_value())
                {
                    permanentModifiers.push_back(ResolvedModifier{
                        .modifier = &modifier,
                        .value = modifier.resolveValue(resourceManager)
                    });
                }
            }
        }
        return calculateResolvedTotal(permanentModifiers);
    }

    ModifierSetView ModifierSet::toView(ResourceManager &resourceManager) const
    {
        return toView(std::vector<const ModifierSet *>{this}, resourceManager);
    }

    ModifierSetView ModifierSet::toView(const std::vector<const ModifierSet *> &modifierSets, ResourceManager &resourceManager)
    {
        std::vector<ResolvedModifier> permanentModifiers;
        std::vector<ConditionalGroup> conditionalGroups;
        std::vector<ModifierView> modifierViews;

        for (const ModifierSet *modifierSet : modifierSets)
        {
            modifierViews.reserve(modifierViews.size() + modifierSet->modifiers_.size());
            for (const Modifier &modifier : modifierSet->modifiers_)
            {
                const int value = modifier.resolveValue(resourceManager);
                const ResolvedModifier resolvedModifier{
                    .modifier = &modifier,
                    .value = value
                };
                modifierViews.push_back(modifier.toView(value));

                if (!modifier.condition().has_value())
                {
                    permanentModifiers.push_back(resolvedModifier);
                    continue;
                }

                auto group = std::find_if(conditionalGroups.begin(), conditionalGroups.end(), [&modifier](const ConditionalGroup &candidate)
                {
                    return candidate.condition == *modifier.condition();
                });
                if (group == conditionalGroups.end())
                {
                    conditionalGroups.push_back(ConditionalGroup{
                        .condition = *modifier.condition(),
                        .modifiers = {resolvedModifier}
                    });
                }
                else
                {
                    group->modifiers.push_back(resolvedModifier);
                }
            }
        }

        const int permanentTotal = calculateResolvedTotal(permanentModifiers);
        std::vector<ModifierSetView::ConditionalTotalView> conditionalTotals;
        conditionalTotals.reserve(conditionalGroups.size());

        for (const ConditionalGroup &group : conditionalGroups)
        {
            std::vector<ResolvedModifier> applicableModifiers = permanentModifiers;
            applicableModifiers.insert(applicableModifiers.end(), group.modifiers.begin(), group.modifiers.end());
            conditionalTotals.push_back(ModifierSetView::ConditionalTotalView{
                .condition = group.condition,
                .value = calculateResolvedTotal(applicableModifiers) - permanentTotal
            });
        }

        return ModifierSetView{
            .total = permanentTotal,
            .conditionalTotals = std::move(conditionalTotals),
            .modifiers = std::move(modifierViews)
        };
    }
}
