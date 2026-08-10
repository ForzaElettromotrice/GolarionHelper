#pragma once

#include "golarion/resource/modifier.hpp"

#include <string>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct ModifierGroupSaveData;
    struct ModifierGroupView;

    struct TargetedModifier
    {
        std::string resourceName;
        Modifier modifier;
    };

    class ModifierGroup final
    {
    public:
        explicit ModifierGroup(std::vector<TargetedModifier> modifiers);
        explicit ModifierGroup(const ModifierGroupSaveData &data);

        ModifierGroupView toView(ResourceManager &resourceManager) const;
        ModifierGroupSaveData toSaveData() const;
        const std::vector<TargetedModifier> &modifiers() const;

    private:
        std::vector<TargetedModifier> modifiers_;
    };
}
