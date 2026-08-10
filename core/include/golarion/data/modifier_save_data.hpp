#pragma once

#include "golarion/resource/modifier.hpp"

#include <optional>
#include <string>

namespace golarion
{
    struct ModifierSaveData
    {
        std::string id;
        ModifierType type;
        std::string source;
        std::string description;
        std::optional<BonusType> bonusType;
        std::string expression;
        std::optional<std::string> condition;
    };
}
