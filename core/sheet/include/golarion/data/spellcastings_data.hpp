#pragma once

#include "golarion/data/prepared_spellcasting_data.hpp"
#include "golarion/data/spontaneous_spellcasting_data.hpp"
#include "golarion/data/flexible_spellcasting_data.hpp"

#include <variant>

namespace golarion
{
    using SpellcastingStateData = std::variant<PreparedSpellcastingData, SpontaneousSpellcastingData, FlexibleSpellcastingData>;

    struct SpellcastingData
    {
        std::string id;
        std::vector<std::string> repertoire;
        SpellcastingStateData state;
    };

    struct SpellcastingsData
    {
        std::vector<SpellcastingData> castings;
    };
}
