#pragma once

#include "golarion/character/metamagic.hpp"
#include "golarion/character/spell_slot_id.hpp"

#include <vector>

namespace golarion
{
    struct FlexibleSpellPreparationData
    {
        SpellSlotId preparationId;
        std::string spellId;
        std::vector<MetamagicApplication> metamagic;
        std::optional<PreparedMetamagicData> evaluation = std::nullopt;
    };

    struct FlexibleSpellcastingData
    {
        std::vector<FlexibleSpellPreparationData> preparations;
        std::vector<SpellSlotId> consumedSlots;
    };
}
