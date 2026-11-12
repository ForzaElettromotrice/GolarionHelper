#pragma once

#include "golarion/character/metamagic.hpp"
#include "golarion/character/spell_slot_id.hpp"

namespace golarion
{
    struct SpellPreparationDefinition
    {
        std::string spellId;
        std::vector<MetamagicApplication> metamagic{};
    };

    struct PreparedSpellConsumptionDefinition
    {
        std::optional<std::string> replacementSpellId{};
        std::vector<MetamagicApplication> metamagic{};
    };

    struct SpontaneousSpellConsumptionDefinition
    {
        std::string spellId;
        std::vector<MetamagicApplication> metamagic{};
    };

    struct FlexibleSpellConsumptionDefinition
    {
        SpellSlotId preparation;
        std::vector<MetamagicApplication> metamagic{};
        std::optional<std::string> mixingPermissionId{};
    };
}
