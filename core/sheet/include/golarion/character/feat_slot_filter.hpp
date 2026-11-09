#pragma once

#include "golarion/character/feat_type.hpp"

#include <string>
#include <variant>
#include <vector>

namespace golarion
{
    struct AnyFeat {};
    struct FeatTypeFilter { FeatType type; };
    struct FeatIdFilter { std::vector<std::string> ids; };
    using FeatSlotFilter = std::variant<AnyFeat, FeatTypeFilter, FeatIdFilter>;
}
