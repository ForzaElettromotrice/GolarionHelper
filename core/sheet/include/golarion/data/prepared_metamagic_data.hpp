#pragma once

#include "golarion/character/character_class_type.hpp"

namespace golarion
{
    // Historical preparation result, independent from the grants currently available.
    struct PreparedMetamagicData
    {
        int baseSpellLevel;
        int effectiveSpellLevel;
        int requiredSlotLevel;
        int nominalSlotLevel;
        CharacterClassType spellListClass;
        int paidSlots = 1;
    };
}
