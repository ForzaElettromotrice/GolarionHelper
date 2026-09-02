#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    EffectDefinition effectDefinitionFromJson(const nlohmann::json &json);
}
