#include "golarion/resource/modifier_group.hpp"

#include "golarion/data/modifier_group_save_data.hpp"
#include "golarion/view/modifier_group_view.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace
{
    std::vector<golarion::TargetedModifier> targetedModifiers(const golarion::ModifierGroupSaveData &data)
    {
        std::vector<golarion::TargetedModifier> modifiers;
        modifiers.reserve(data.modifiers.size());

        for (const auto &[resourceName, modifier] : data.modifiers)
        {
            modifiers.push_back(golarion::TargetedModifier{
                .resourceName = resourceName,
                .modifier = golarion::Modifier(modifier)
            });
        }

        return modifiers;
    }
}

namespace golarion
{
    ModifierGroup::ModifierGroup(std::vector<TargetedModifier> modifiers) : modifiers_(std::move(modifiers))
    {
        if (modifiers_.empty())
        {
            throw std::invalid_argument("modifier group must not be empty");
        }

        std::unordered_set<std::string> modifierIds;
        for (auto &[resourceName, modifier]: modifiers_)
        {
            resourceName = normalize(resourceName);
            if (!modifierIds.insert(modifier.id()).second)
            {
                throw std::invalid_argument("modifier is duplicated in group: " + modifier.id());
            }
        }
    }

    ModifierGroup::ModifierGroup(const ModifierGroupSaveData &data) : ModifierGroup(targetedModifiers(data))
    {
    }

    ModifierGroupView ModifierGroup::toView(ResourceManager &resourceManager) const
    {
        std::vector<ModifierGroupView::TargetedModifierView> modifierViews;
        modifierViews.reserve(modifiers_.size());

        for (const auto &[resourceName, modifier]: modifiers_)
        {
            modifierViews.push_back(ModifierGroupView::TargetedModifierView{
                .resourceName = resourceName,
                .modifier = modifier.toView(resourceManager)
            });
        }

        return ModifierGroupView{.modifiers = std::move(modifierViews)};
    }

    ModifierGroupSaveData ModifierGroup::toSaveData() const
    {
        std::vector<ModifierGroupSaveData::TargetedModifierSaveData> modifierData;
        modifierData.reserve(modifiers_.size());

        for (const auto &[resourceName, modifier]: modifiers_)
        {
            modifierData.push_back(ModifierGroupSaveData::TargetedModifierSaveData{
                .resourceName = resourceName,
                .modifier = modifier.toSaveData()
            });
        }

        return ModifierGroupSaveData{.modifiers = std::move(modifierData)};
    }

    const std::vector<TargetedModifier> &ModifierGroup::modifiers() const
    {
        return modifiers_;
    }
}
