#pragma once

#include "golarion/resource/modifier.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct ModifierSetView;

    class ModifierSet final
    {
    public:
        void addModifier(Modifier modifier);
        void removeModifier(std::string_view modifierId);
        bool empty() const;
        int calculateTotal(ResourceManager &resourceManager) const;
        ModifierSetView toView(ResourceManager &resourceManager) const;

    private:
        friend class ResourceManager;

        static int calculateTotal(const std::vector<const ModifierSet *> &modifierSets, ResourceManager &resourceManager);
        static ModifierSetView toView(const std::vector<const ModifierSet *> &modifierSets, ResourceManager &resourceManager);
        static ModifierSetView toView(const std::vector<const ModifierSet *> &modifierSets, ResourceManager &resourceManager, const std::vector<std::string> &activeConditions);

        std::vector<Modifier> modifiers_;
    };
}
