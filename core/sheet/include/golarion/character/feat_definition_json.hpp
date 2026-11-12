#pragma once

#include "golarion/character/feat_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json featDefinitionToJson(const FeatDefinition &definition);
    FeatDefinition featDefinitionFromJson(const nlohmann::json &json);
}
