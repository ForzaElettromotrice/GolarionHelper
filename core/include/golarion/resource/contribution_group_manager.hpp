#pragma once

#include "golarion/resource/contribution_group.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace golarion
{
    class ResourceManager;
    struct ContributionGroupManagerSaveData;
    struct ContributionGroupManagerView;

    class ContributionGroupManager final
    {
    public:
        explicit ContributionGroupManager(ResourceManager &resourceManager);
        ~ContributionGroupManager();

        ContributionGroupManager(const ContributionGroupManager &) = delete;
        ContributionGroupManager &operator=(const ContributionGroupManager &) = delete;
        ContributionGroupManager(ContributionGroupManager &&) = delete;
        ContributionGroupManager &operator=(ContributionGroupManager &&) = delete;

        void addGroup(const std::string &groupId, ContributionGroup group, bool enabled);
        void removeGroup(std::string_view groupId);
        void setGroupEnabled(std::string_view groupId, bool enabled);
        ContributionGroupManagerView toView() const;
        ContributionGroupManagerSaveData toSaveData() const;

    private:
        struct ManagedGroup
        {
            ContributionGroup group;
            bool enabled;
        };

        ManagedGroup &managedGroup(std::string_view groupId);
        void activate(const ContributionGroup &group) const;
        void deactivate(const ContributionGroup &group) const;

        ResourceManager &resourceManager_;
        std::unordered_map<std::string, ManagedGroup> groups_;
    };
}
