#include "golarion/resource/modifier_group_manager.hpp"

#include <algorithm>
#include <optional>
#include <ranges>

#include "golarion/data/modifier_group_manager_save_data.hpp"
#include "golarion/view/modifier_group_manager_view.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    ModifierGroupManager::ModifierGroupManager(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
    }

    ModifierGroupManager::~ModifierGroupManager()
    {
        for (const auto &[group, enabled]: groups_ | std::views::values)
        {
            if (!enabled)
            {
                continue;
            }

            try
            {
                deactivate(group);
            } catch (...)
            {
            }
        }
    }

    void ModifierGroupManager::addGroup(const std::string &groupId, ModifierGroup group, bool enabled)
    {
        std::string normalizedGroupId = normalize(groupId);
        auto [entry, inserted] = groups_.emplace(normalizedGroupId, ManagedGroup{.group = std::move(group), .enabled = false});
        if (!inserted)
        {
            throw std::invalid_argument("modifier group is already registered: " + normalizedGroupId);
        }

        if (!enabled)
        {
            return;
        }

        try
        {
            activate(entry->second.group);
            entry->second.enabled = true;
        } catch (...)
        {
            groups_.erase(entry);
            throw;
        }
    }

    void ModifierGroupManager::removeGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("modifier group is not registered: " + normalizedGroupId);
        }

        if (entry->second.enabled)
        {
            deactivate(entry->second.group);
        }
        groups_.erase(entry);
    }

    void ModifierGroupManager::removeModifiersForResource(std::string_view resourceName)
    {
        const std::string normalizedResourceName = normalize(resourceName);

        for (auto entry = groups_.begin(); entry != groups_.end();)
        {
            std::vector<TargetedModifier> remainingModifiers;
            std::vector<TargetedModifier> removedModifiers;
            remainingModifiers.reserve(entry->second.group.modifiers().size());

            for (const TargetedModifier &targetedModifier : entry->second.group.modifiers())
            {
                if (targetedModifier.resourceName == normalizedResourceName)
                {
                    removedModifiers.push_back(targetedModifier);
                } else
                {
                    remainingModifiers.push_back(targetedModifier);
                }
            }

            if (removedModifiers.empty())
            {
                ++entry;
                continue;
            }

            std::optional<ModifierGroup> replacement;
            if (!remainingModifiers.empty())
            {
                replacement.emplace(std::move(remainingModifiers));
            }

            if (entry->second.enabled)
            {
                std::size_t removedCount = 0;
                try
                {
                    for (const TargetedModifier &targetedModifier : removedModifiers)
                    {
                        resourceManager_.removeModifier(targetedModifier.resourceName, targetedModifier.modifier.id());
                        ++removedCount;
                    }
                } catch (...)
                {
                    while (removedCount > 0)
                    {
                        --removedCount;
                        const TargetedModifier &targetedModifier = removedModifiers[removedCount];
                        try
                        {
                            resourceManager_.addModifier(targetedModifier.resourceName, targetedModifier.modifier);
                        } catch (...)
                        {
                        }
                    }
                    throw;
                }
            }

            if (!replacement)
            {
                entry = groups_.erase(entry);
                continue;
            }

            entry->second.group = std::move(*replacement);
            ++entry;
        }
    }

    void ModifierGroupManager::setGroupEnabled(std::string_view groupId, bool enabled)
    {
        ManagedGroup &entry = managedGroup(groupId);
        if (entry.enabled == enabled)
        {
            return;
        }

        if (enabled)
        {
            activate(entry.group);
        } else
        {
            deactivate(entry.group);
        }
        entry.enabled = enabled;
    }

    ModifierGroupManagerView ModifierGroupManager::toView() const
    {
        std::vector<ModifierGroupManagerView::GroupView> groupViews;
        groupViews.reserve(groups_.size());

        for (const auto &[groupId, managedGroup]: groups_)
        {
            groupViews.push_back(ModifierGroupManagerView::GroupView{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toView(resourceManager_)
            });
        }

        std::ranges::sort(groupViews, {}, &ModifierGroupManagerView::GroupView::id);
        return ModifierGroupManagerView{.groups = std::move(groupViews)};
    }

    ModifierGroupManagerSaveData ModifierGroupManager::toSaveData() const
    {
        std::vector<ModifierGroupManagerSaveData::GroupSaveData> groupData;
        groupData.reserve(groups_.size());

        for (const auto &[groupId, managedGroup]: groups_)
        {
            groupData.push_back(ModifierGroupManagerSaveData::GroupSaveData{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toSaveData()
            });
        }

        std::ranges::sort(groupData, {}, &ModifierGroupManagerSaveData::GroupSaveData::id);
        return ModifierGroupManagerSaveData{.groups = std::move(groupData)};
    }

    ModifierGroupManager::ManagedGroup &ModifierGroupManager::managedGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("modifier group is not registered: " + normalizedGroupId);
        }
        return entry->second;
    }

    void ModifierGroupManager::activate(const ModifierGroup &group) const
    {
        std::size_t appliedModifiers = 0;
        try
        {
            for (const TargetedModifier &targetedModifier: group.modifiers())
            {
                resourceManager_.addModifier(targetedModifier.resourceName, targetedModifier.modifier);
                ++appliedModifiers;
            }
        } catch (...)
        {
            while (appliedModifiers > 0)
            {
                --appliedModifiers;
                const TargetedModifier &targetedModifier = group.modifiers()[appliedModifiers];
                try
                {
                    resourceManager_.removeModifier(targetedModifier.resourceName, targetedModifier.modifier.id());
                } catch (...)
                {
                }
            }
            throw;
        }
    }

    void ModifierGroupManager::deactivate(const ModifierGroup &group) const
    {
        for (const TargetedModifier &targetedModifier: group.modifiers())
        {
            resourceManager_.removeModifier(targetedModifier.resourceName, targetedModifier.modifier.id());
        }
    }
}
