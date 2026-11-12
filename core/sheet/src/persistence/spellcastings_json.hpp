#pragma once

#include "golarion/data/spellcastings_data.hpp"
#include <nlohmann/json_fwd.hpp>

namespace golarion::persistence
{
    nlohmann::json spellcastingsToJson(const SpellcastingsData &data, bool requireSnapshots = false);
    SpellcastingsData spellcastingsFromJson(const nlohmann::json &json, bool requireSnapshots = false);
    nlohmann::json metamagicUsesToJson(const std::optional<std::map<std::string, int>> &uses);
    std::map<std::string, int> metamagicUsesFromJson(const nlohmann::json &json);
}
