#pragma once

#include "golarion/character/race_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    RaceDefinition raceDefinitionFromJson(const nlohmann::json &json);
}
