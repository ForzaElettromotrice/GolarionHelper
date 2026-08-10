#include "golarion/character/movement_group_manager.hpp"

#include "golarion/data/movement_group_manager_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/movement_group_manager_view.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace golarion
{
    MovementGroupManager::MovementGroupManager(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
    }

    MovementGroupManager::~MovementGroupManager()
    {
        for (const auto &[group, enabled] : groups_ | std::views::values)
        {
            if (!enabled)
            {
                continue;
            }
            try
            {
                deactivate(group);
            }
            catch (...)
            {
            }
        }
    }

    void MovementGroupManager::addGroup(const std::string &groupId, MovementGroup group, bool enabled)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto [entry, inserted] = groups_.emplace(normalizedGroupId, ManagedGroup{.group = std::move(group), .enabled = false});
        if (!inserted)
        {
            throw std::invalid_argument("movement group is already registered: " + normalizedGroupId);
        }
        if (!enabled)
        {
            return;
        }

        try
        {
            activate(entry->second.group);
            entry->second.enabled = true;
        }
        catch (...)
        {
            groups_.erase(entry);
            throw;
        }
    }

    void MovementGroupManager::removeGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("movement group is not registered: " + normalizedGroupId);
        }
        if (entry->second.enabled)
        {
            deactivate(entry->second.group);
        }
        groups_.erase(entry);
    }

    void MovementGroupManager::setGroupEnabled(std::string_view groupId, bool enabled)
    {
        ManagedGroup &entry = managedGroup(groupId);
        if (entry.enabled == enabled)
        {
            return;
        }

        if (enabled)
        {
            activate(entry.group);
        }
        else
        {
            deactivate(entry.group);
        }
        entry.enabled = enabled;
    }

    MovementGroupManagerView MovementGroupManager::toView() const
    {
        std::vector<MovementGroupManagerView::GroupView> groups;
        groups.reserve(groups_.size());
        for (const auto &[groupId, managedGroup] : groups_)
        {
            groups.push_back(MovementGroupManagerView::GroupView{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toView()
            });
        }
        std::ranges::sort(groups, {}, &MovementGroupManagerView::GroupView::id);
        return MovementGroupManagerView{.groups = std::move(groups)};
    }

    MovementGroupManagerSaveData MovementGroupManager::toSaveData() const
    {
        std::vector<MovementGroupManagerSaveData::GroupSaveData> groups;
        groups.reserve(groups_.size());
        for (const auto &[groupId, managedGroup] : groups_)
        {
            groups.push_back(MovementGroupManagerSaveData::GroupSaveData{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toSaveData()
            });
        }
        std::ranges::sort(groups, {}, &MovementGroupManagerSaveData::GroupSaveData::id);
        return MovementGroupManagerSaveData{.groups = std::move(groups)};
    }

    MovementGroupManager::ManagedGroup &MovementGroupManager::managedGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("movement group is not registered: " + normalizedGroupId);
        }
        return entry->second;
    }

    void MovementGroupManager::activate(const MovementGroup &group) const
    {
        std::size_t appliedGrants = 0;
        std::size_t appliedAdjustments = 0;
        try
        {
            for (const MovementGrant &grant : group.grants())
            {
                resourceManager_.addToCollection(MovementGrantsResource, grant);
                ++appliedGrants;
            }
            for (const MovementAdjustment &adjustment : group.adjustments())
            {
                resourceManager_.addToCollection(MovementAdjustmentsResource, adjustment);
                ++appliedAdjustments;
            }
        }
        catch (...)
        {
            while (appliedAdjustments > 0)
            {
                --appliedAdjustments;
                try
                {
                    resourceManager_.removeFromCollection(MovementAdjustmentsResource, group.adjustments()[appliedAdjustments].toSaveData().id);
                }
                catch (...)
                {
                }
            }
            while (appliedGrants > 0)
            {
                --appliedGrants;
                try
                {
                    resourceManager_.removeFromCollection(MovementGrantsResource, group.grants()[appliedGrants].toSaveData().id);
                }
                catch (...)
                {
                }
            }
            throw;
        }
    }

    void MovementGroupManager::deactivate(const MovementGroup &group) const
    {
        std::vector<MovementAdjustment> removedAdjustments;
        removedAdjustments.reserve(group.adjustments().size());
        try
        {
            for (auto adjustment = group.adjustments().rbegin(); adjustment != group.adjustments().rend(); ++adjustment)
            {
                resourceManager_.removeFromCollection(MovementAdjustmentsResource, adjustment->toSaveData().id);
                removedAdjustments.push_back(*adjustment);
            }
        }
        catch (...)
        {
            for (auto adjustment = removedAdjustments.rbegin(); adjustment != removedAdjustments.rend(); ++adjustment)
            {
                try
                {
                    resourceManager_.addToCollection(MovementAdjustmentsResource, *adjustment);
                }
                catch (...)
                {
                }
            }
            throw;
        }

        std::vector<MovementGrant> removedGrants;
        removedGrants.reserve(group.grants().size());
        try
        {
            for (auto grant = group.grants().rbegin(); grant != group.grants().rend(); ++grant)
            {
                resourceManager_.removeFromCollection(MovementGrantsResource, grant->toSaveData().id);
                removedGrants.push_back(*grant);
            }
        }
        catch (...)
        {
            for (auto grant = removedGrants.rbegin(); grant != removedGrants.rend(); ++grant)
            {
                try
                {
                    resourceManager_.addToCollection(MovementGrantsResource, *grant);
                }
                catch (...)
                {
                }
            }
            for (const MovementAdjustment &adjustment : group.adjustments())
            {
                try
                {
                    resourceManager_.addToCollection(MovementAdjustmentsResource, adjustment);
                }
                catch (...)
                {
                }
            }
            throw;
        }
    }
}
