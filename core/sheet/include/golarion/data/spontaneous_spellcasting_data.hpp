#pragma once

#include "golarion/character/spell_slot_id.hpp"

#include <vector>

namespace golarion
{
    struct SpontaneousSpellcastingData
    {
        std::vector<SpellSlotId> consumedSlots;
    };
}
