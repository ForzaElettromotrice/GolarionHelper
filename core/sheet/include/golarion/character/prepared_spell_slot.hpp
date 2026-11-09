#pragma once
#include "golarion/character/metamagic.hpp"
#include "golarion/character/spell_slot_id.hpp"

#include <compare>
#include <cstddef>
#include <optional>
#include <string>

namespace golarion
{
    enum class PreparedSpellSlotState
    {
        Free,
        Prepared,
        Consumed
    };

    struct PreparedSpellSlot
    {
        PreparedSpellSlotState state = PreparedSpellSlotState::Free;
        std::optional<std::string> spellId;
        std::vector<MetamagicApplication> metamagic;
    };
}
