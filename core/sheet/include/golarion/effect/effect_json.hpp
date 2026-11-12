#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json effectDefinitionToJson(const EffectDefinition &definition);
    EffectDefinition effectDefinitionFromJson(const nlohmann::json &json);
}
