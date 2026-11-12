#pragma once

#include "golarion/data/enhancements_data.hpp"

namespace golarion
{
    struct EnhancementView
    {
        EnhancementData data;
        // Raw expression value, not the result of stacking with other sources.
        std::optional<int> resolvedValue;
    };

    struct EnhancementGroupView
    {
        std::string id;
        std::string name;
        std::vector<EnhancementView> enhancements;
    };

    struct EnhancementsView
    {
        std::vector<EnhancementGroupView> groups;
    };
}
