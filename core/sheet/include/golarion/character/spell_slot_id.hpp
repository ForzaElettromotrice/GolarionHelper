#pragma once

#include <compare>
#include <cstddef>
#include <string>

namespace golarion
{
    struct SpellSlotId
    {
        std::string groupId;
        int spellLevel;
        std::size_t index;

        auto operator<=>(const SpellSlotId &) const = default;
    };
}
