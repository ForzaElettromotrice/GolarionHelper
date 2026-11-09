#pragma once

#include "golarion/character/prepared_spell_slot.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct PreparedSpellData
    {
        SpellSlotId slot;
        std::string spellId;
        bool consumed = false;
        std::vector<SpellSlotId> additionalSlots;
        std::vector<MetamagicApplication> metamagic;
    };

    struct PreparedSpellcastingData
    {
        std::vector<PreparedSpellData> preparations;
    };
}
