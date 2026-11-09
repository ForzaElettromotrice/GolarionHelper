#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/prepared_spell_slot.hpp"
#include "golarion/character/spell_slot_grant.hpp"

#include <vector>

namespace golarion
{
    struct PreparedSpellSlotView
    {
        SpellSlotId slot;
        PreparedSpellSlotState state;
        std::optional<std::string> spellId;
        std::optional<SpellSlotId> preparationSlot;
        std::vector<MetamagicApplication> metamagic;
    };

    struct PreparedSpellSlotGroupView
    {
        SpellSlotGrant grant;
        std::vector<PreparedSpellSlotView> slots;
    };

    struct PreparedSpellcastingView
    {
        CharacterClassType spellListClass;
        std::vector<PreparedSpellSlotGroupView> groups;
    };
}
