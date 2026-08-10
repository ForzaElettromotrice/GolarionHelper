#include "golarion/resource/contribution_group_manager.hpp"

#include "golarion/data/contribution_group_manager_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/contribution_group_manager_view.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace golarion
{
    ContributionGroupManager::ContributionGroupManager(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
    }

    ContributionGroupManager::~ContributionGroupManager()
    {
        for (const ManagedGroup &managedGroup : groups_ | std::views::values)
        {
            if (!managedGroup.enabled)
            {
                continue;
            }

            try
            {
                deactivate(managedGroup.group);
            }
            catch (...)
            {
            }
        }
    }

    void ContributionGroupManager::addGroup(const std::string &groupId, ContributionGroup group, bool enabled)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto [entry, inserted] = groups_.emplace(normalizedGroupId, ManagedGroup{.group = std::move(group), .enabled = false});
        if (!inserted)
        {
            throw std::invalid_argument("contribution group is already registered: " + normalizedGroupId);
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

    void ContributionGroupManager::removeGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("contribution group is not registered: " + normalizedGroupId);
        }

        if (entry->second.enabled)
        {
            deactivate(entry->second.group);
        }
        groups_.erase(entry);
    }

    void ContributionGroupManager::setGroupEnabled(std::string_view groupId, bool enabled)
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

    ContributionGroupManagerView ContributionGroupManager::toView() const
    {
        std::vector<ContributionGroupManagerView::GroupView> groupViews;
        groupViews.reserve(groups_.size());

        for (const auto &[groupId, managedGroup] : groups_)
        {
            groupViews.push_back(ContributionGroupManagerView::GroupView{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toView(resourceManager_)
            });
        }

        std::ranges::sort(groupViews, {}, &ContributionGroupManagerView::GroupView::id);
        return ContributionGroupManagerView{.groups = std::move(groupViews)};
    }

    ContributionGroupManagerSaveData ContributionGroupManager::toSaveData() const
    {
        std::vector<ContributionGroupManagerSaveData::GroupSaveData> groupData;
        groupData.reserve(groups_.size());

        for (const auto &[groupId, managedGroup] : groups_)
        {
            groupData.push_back(ContributionGroupManagerSaveData::GroupSaveData{
                .id = groupId,
                .enabled = managedGroup.enabled,
                .group = managedGroup.group.toSaveData()
            });
        }

        std::ranges::sort(groupData, {}, &ContributionGroupManagerSaveData::GroupSaveData::id);
        return ContributionGroupManagerSaveData{.groups = std::move(groupData)};
    }

    ContributionGroupManager::ManagedGroup &ContributionGroupManager::managedGroup(std::string_view groupId)
    {
        const std::string normalizedGroupId = normalize(groupId);
        auto entry = groups_.find(normalizedGroupId);
        if (entry == groups_.end())
        {
            throw std::invalid_argument("contribution group is not registered: " + normalizedGroupId);
        }
        return entry->second;
    }

    void ContributionGroupManager::activate(const ContributionGroup &group) const
    {
        std::size_t appliedContributions = 0;
        try
        {
            for (const TargetedContribution &targetedContribution : group.contributions())
            {
                resourceManager_.addContribution(targetedContribution.resourceName, targetedContribution.contribution);
                ++appliedContributions;
            }
        }
        catch (...)
        {
            while (appliedContributions > 0)
            {
                --appliedContributions;
                const TargetedContribution &targetedContribution = group.contributions()[appliedContributions];
                try
                {
                    resourceManager_.removeContribution(targetedContribution.resourceName, targetedContribution.contribution.id_);
                }
                catch (...)
                {
                }
            }
            throw;
        }
    }

    void ContributionGroupManager::deactivate(const ContributionGroup &group) const
    {
        std::size_t removedContributions = 0;
        try
        {
            for (const TargetedContribution &targetedContribution : group.contributions())
            {
                resourceManager_.removeContribution(targetedContribution.resourceName, targetedContribution.contribution.id_);
                ++removedContributions;
            }
        }
        catch (...)
        {
            while (removedContributions > 0)
            {
                --removedContributions;
                const TargetedContribution &targetedContribution = group.contributions()[removedContributions];
                try
                {
                    resourceManager_.addContribution(targetedContribution.resourceName, targetedContribution.contribution);
                }
                catch (...)
                {
                }
            }
            throw;
        }
    }
}
