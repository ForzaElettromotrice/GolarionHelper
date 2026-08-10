#pragma once

#include "golarion/resource/modifier_group.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace golarion
{
    class ResourceManager;
    struct ModifierGroupManagerSaveData;
    struct ModifierGroupManagerView;

    class ModifierGroupManager final
    {
    public:
        explicit ModifierGroupManager(ResourceManager &resourceManager);
        ~ModifierGroupManager();

        ModifierGroupManager(const ModifierGroupManager &) = delete;
        ModifierGroupManager &operator=(const ModifierGroupManager &) = delete;
        ModifierGroupManager(ModifierGroupManager &&) = delete;
        ModifierGroupManager &operator=(ModifierGroupManager &&) = delete;

        void addGroup(const std::string &groupId, ModifierGroup group, bool enabled);
        void removeGroup(std::string_view groupId);
        void removeModifiersForResource(std::string_view resourceName);
        void setGroupEnabled(std::string_view groupId, bool enabled);
        ModifierGroupManagerView toView() const;
        ModifierGroupManagerSaveData toSaveData() const;

    private:
        struct ManagedGroup
        {
            ModifierGroup group;
            bool enabled;
        };

        ManagedGroup &managedGroup(std::string_view groupId);
        void activate(const ModifierGroup &group) const;
        void deactivate(const ModifierGroup &group) const;

        ResourceManager &resourceManager_;
        std::unordered_map<std::string, ManagedGroup> groups_;
    };
}
