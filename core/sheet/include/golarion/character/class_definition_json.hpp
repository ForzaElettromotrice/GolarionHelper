#pragma once

#include "golarion/character/class_definition.hpp"

#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json classDefinitionToJson(const ClassDefinition &definition);
    ClassDefinition classDefinitionFromJson(const nlohmann::json &json);
}
