#pragma once

#include "golarion/view/condition_view.hpp"

#include <vector>

namespace golarion
{
    struct ConditionManagerView
    {
        std::vector<ConditionView> conditions;
    };
}
