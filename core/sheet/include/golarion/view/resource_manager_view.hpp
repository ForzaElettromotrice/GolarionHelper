#pragma once

#include "golarion/view/modifier_set_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ResourceManagerView
    {
        struct TargetView
        {
            std::string name;
            int value;
        };

        struct EnhanceableResourceView
        {
            std::string name;
            std::vector<std::string> parentResources;
            ModifierSetView modifiers;
        };

        std::vector<TargetView> targets;
        std::vector<EnhanceableResourceView> enhanceableResources;
        std::vector<std::string> collections;
    };
}
