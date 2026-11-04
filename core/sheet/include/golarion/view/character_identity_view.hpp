#pragma once

#include "golarion/character/character_identity.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace golarion
{
    struct CharacterIdentityView
    {
        std::string name;
        std::string playerName;
        std::optional<Alignment> alignment;
        std::optional<std::string> deity;
        std::optional<std::string> homeland;
        std::optional<std::string> gender;
        std::optional<int> age;
        std::optional<int> heightCentimeters;
        std::optional<std::int64_t> weightGrams;
        std::optional<std::string> hair;
        std::optional<std::string> eyes;
        std::optional<std::string> appearance;
    };
}
