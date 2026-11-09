#pragma once

#include "golarion/data/spellcastings_data.hpp"
#include <nlohmann/json_fwd.hpp>

namespace golarion::persistence
{
    nlohmann::json spellcastingsToJson(const SpellcastingsData &data);
    SpellcastingsData spellcastingsFromJson(const nlohmann::json &json);
}
