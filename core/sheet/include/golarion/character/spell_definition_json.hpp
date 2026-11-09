#pragma once

#include "golarion/character/spell_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    SpellDefinition spellDefinitionFromJson(const nlohmann::json &json);
}
