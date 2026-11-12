#pragma once

#include "golarion/data/prepared_spellcasting_data.hpp"
#include "golarion/data/spontaneous_spellcasting_data.hpp"
#include "golarion/data/flexible_spellcasting_data.hpp"

#include "golarion/data/spell_repertoire_data.hpp"

#include <variant>
#include <map>
#include <optional>

namespace golarion
{
    using SpellcastingStateData = std::variant<PreparedSpellcastingData, SpontaneousSpellcastingData, FlexibleSpellcastingData>;

    struct SpellcastingData
    {
        std::string id;
        // Legacy flat knowledge; repertoireData is authoritative when present.
        std::vector<std::string> repertoire;
        SpellcastingStateData state;
        std::optional<SpellRepertoireData> repertoireData = std::nullopt;
    };

    struct SpellcastingsData
    {
        std::vector<SpellcastingData> castings;
        // Absent only for legacy saves; shared source pools are stored once, by stable ID.
        std::optional<std::map<std::string, int>> metamagicUses = std::nullopt;
    };
}
