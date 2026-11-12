#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/spell_slot_id.hpp"
#include "golarion/character/spell_slot_grant.hpp"
#include "golarion/view/metamagic_view.hpp"

#include <vector>

namespace golarion
{
    enum class SpontaneousSpellSlotState { Free, Consumed };

    struct SpontaneousSpellSlotView
    {
        SpellSlotId slot;
        SpontaneousSpellSlotState state;
    };

    struct SpontaneousSpellSlotGroupView
    {
        SpellSlotGrant grant;
        std::vector<SpontaneousSpellSlotView> slots;
    };

    struct SpontaneousSpellcastingView
    {
        CharacterClassType spellListClass;
        std::vector<SpontaneousSpellSlotGroupView> groups;
        std::vector<MetamagicGrantView> metamagic{};
    };
}
