#pragma once

#include "golarion/view/modifier_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ModifierGroupView
    {
        struct TargetedModifierView
        {
            std::string resourceName;
            ModifierView modifier;
        };

        std::vector<TargetedModifierView> modifiers;
    };
}
