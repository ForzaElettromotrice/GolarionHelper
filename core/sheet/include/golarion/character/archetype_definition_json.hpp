#pragma once

#include "golarion/character/archetype_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json archetypeDefinitionToJson(const ArchetypeDefinition &definition);
    ArchetypeDefinition archetypeDefinitionFromJson(const nlohmann::json &json);
}
