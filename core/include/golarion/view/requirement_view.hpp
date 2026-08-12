#pragma once

#include <string>

namespace golarion
{
    struct RequirementView
    {
        std::string expression;
        std::string failureReason;
        bool satisfied;
    };
}
