#pragma once

#include <string>

namespace golarion
{
    struct ContributionView
    {
        std::string id;
        std::string expression;
        int resolvedValue;
    };
}
