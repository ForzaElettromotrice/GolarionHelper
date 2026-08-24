#pragma once

#include "golarion/view/modifier_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ModifierSetView
    {
        struct ConditionalTotalView
        {
            std::string condition;
            int value;
        };

        int permanentTotal;
        int total;
        std::vector<ConditionalTotalView> conditionalTotals;
        std::vector<ModifierView> modifiers;
    };
}
