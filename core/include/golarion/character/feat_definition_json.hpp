#pragma once

#include "golarion/character/feat_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    FeatDefinition featDefinitionFromJson(const nlohmann::json &json);
}
