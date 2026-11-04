#pragma once

#include <cstddef>
#include <string_view>

namespace golarion
{
    enum class EquipmentSlot
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

    std::string_view displayName(EquipmentSlot slot);
    std::size_t capacity(EquipmentSlot slot);
}
