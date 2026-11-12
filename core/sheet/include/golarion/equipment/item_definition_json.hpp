#pragma once
#include "golarion/equipment/item.hpp"
#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json itemDefinitionToJson(const ItemDefinition &definition);
    ItemDefinition itemDefinitionFromJson(const nlohmann::json &json);
}
