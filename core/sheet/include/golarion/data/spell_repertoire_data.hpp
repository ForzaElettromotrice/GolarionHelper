#pragma once
#include "golarion/character/spell_repertoire_definition.hpp"
#include <vector>

namespace golarion
{
    struct SpellRepertoireData
    {
        std::vector<std::string> manualSpellIds;
        std::vector<RepertoireSelectionDefinition> selections;
        // Legacy spontaneous knowledge has no source allocation; keep it for reassignment.
        std::vector<std::string> unassignedSpellIds;
    };
}
