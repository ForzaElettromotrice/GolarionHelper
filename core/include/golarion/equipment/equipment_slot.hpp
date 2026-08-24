#pragma once

#include <cstddef>
#include <string_view>

namespace golarion
{
    enum class MagicItemSlot
    {
        Ring,
        Armor,
        Belt,
        Neck,
        Body,
        Headband,
        Hands,
        Eyes,
        Feet,
        Wrists,
        Shield,
        Shoulders,
        Head,
        Chest
    };

    enum class HandSlot
    {
        Main,
        Off
    };

    enum class HandUsage
    {
        None,
        OneHand,
        TwoHands
    };

    std::string_view displayName(MagicItemSlot slot);
    std::string_view displayName(HandSlot slot);
    std::string_view displayName(HandUsage usage);
    std::size_t capacity(MagicItemSlot slot);
}
