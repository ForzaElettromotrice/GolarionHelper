#pragma once

#include "golarion/resource/modifier.hpp"
#include "golarion/view/requirement_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct ModifierView
    {
        std::string id;
        ModifierType type;
        std::string source;
        std::string description;
        std::optional<BonusType> bonusType;
        std::string expression;
        std::optional<std::string> condition;
        int resolvedValue;
        bool active;
        std::vector<RequirementView> requirements;
    };
}
