#pragma once

#include "golarion/character/prepared_spell_slot.hpp"
#include "golarion/data/prepared_metamagic_data.hpp"

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
        std::optional<PreparedMetamagicData> evaluation = std::nullopt;
    };

    struct PreparedSpellcastingData
    {
        std::vector<PreparedSpellData> preparations;
    };
}
