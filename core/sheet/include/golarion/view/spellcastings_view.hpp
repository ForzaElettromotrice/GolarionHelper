#pragma once

#include "golarion/character/spellcasting_mode.hpp"
#include "golarion/view/prepared_spellcasting_view.hpp"
#include "golarion/view/spontaneous_spellcasting_view.hpp"
#include "golarion/view/flexible_spellcasting_view.hpp"

#include "golarion/view/spell_repertoire_view.hpp"

#include <string>
#include <variant>
#include <vector>

namespace golarion
{
    using SpellcastingStateView = std::variant<PreparedSpellcastingView, SpontaneousSpellcastingView, FlexibleSpellcastingView>;

    struct SpellcastingView
    {
        std::string id;
        std::string source;
        SpellcastingMode mode;
        std::vector<std::string> repertoire;
        SpellcastingStateView state;
        SpellRepertoireView repertoireDetails{};
    };

    struct SpellcastingsView
    {
        std::vector<SpellcastingView> castings;
        std::vector<MetamagicUsagePoolView> metamagicPools{};
    };
}
