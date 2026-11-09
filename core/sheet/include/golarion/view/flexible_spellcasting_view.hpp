#pragma once

#include "golarion/character/metamagic.hpp"
#include "golarion/view/spontaneous_spellcasting_view.hpp"

namespace golarion
{
    struct FlexibleSpellPreparationView
    {
        SpellSlotId preparationId;
        std::optional<std::string> spellId;
        std::vector<MetamagicApplication> metamagic;
    };

    struct FlexiblePreparationGroupView
    {
        SpellSlotGrant grant;
        std::vector<FlexibleSpellPreparationView> preparations;
    };

    struct FlexibleSpellcastingView
    {
        CharacterClassType spellListClass;
        std::vector<FlexiblePreparationGroupView> preparationGroups;
        std::vector<SpontaneousSpellSlotGroupView> slotGroups;
    };
}
