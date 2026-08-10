#pragma once

#include "golarion/data/modifier_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ModifierGroupSaveData
    {
        struct TargetedModifierSaveData
        {
            std::string resourceName;
            ModifierSaveData modifier;
        };

        std::vector<TargetedModifierSaveData> modifiers;
    };
}
