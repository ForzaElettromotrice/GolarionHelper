#pragma once

#include "golarion/data/prepared_spellcasting_data.hpp"
#include "golarion/data/spontaneous_spellcasting_data.hpp"
#include "golarion/data/flexible_spellcasting_data.hpp"

#include <variant>
#include <map>
#include <optional>

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
        // Absent only for legacy saves; shared source pools are stored once, by stable ID.
        std::optional<std::map<std::string, int>> metamagicUses = std::nullopt;
    };
}
