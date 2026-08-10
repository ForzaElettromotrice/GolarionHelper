#pragma once

#include "golarion/character/movement_group.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace golarion
{
    class ResourceManager;
    struct MovementGroupManagerSaveData;
    struct MovementGroupManagerView;

    class MovementGroupManager final
    {
    public:
        explicit MovementGroupManager(ResourceManager &resourceManager);
        ~MovementGroupManager();

        MovementGroupManager(const MovementGroupManager &) = delete;
        MovementGroupManager &operator=(const MovementGroupManager &) = delete;
        MovementGroupManager(MovementGroupManager &&) = delete;
        MovementGroupManager &operator=(MovementGroupManager &&) = delete;

        void addGroup(const std::string &groupId, MovementGroup group, bool enabled);
        void removeGroup(std::string_view groupId);
        void setGroupEnabled(std::string_view groupId, bool enabled);
        MovementGroupManagerView toView() const;
        MovementGroupManagerSaveData toSaveData() const;

    private:
        struct ManagedGroup
        {
            MovementGroup group;
            bool enabled;
        };

        ManagedGroup &managedGroup(std::string_view groupId);
        void activate(const MovementGroup &group) const;
        void deactivate(const MovementGroup &group) const;

        ResourceManager &resourceManager_;
        std::unordered_map<std::string, ManagedGroup> groups_;
    };
}
