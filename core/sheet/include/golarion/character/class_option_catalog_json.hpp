#pragma once

#include "golarion/character/class_definition.hpp"

#include <nlohmann/json_fwd.hpp>

#include <vector>

namespace golarion
{
    std::vector<ClassDefinition> classDefinitionsWithOptionsFromJson(const nlohmann::json &definitions, const nlohmann::json &optionCatalogs);
}
