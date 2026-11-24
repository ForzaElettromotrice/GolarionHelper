#pragma once

#include <string_view>
#include <nlohmann/json.hpp>

namespace golarion::embedded
{
    // Test-only catalog injection; the production catalog is unchanged.
    inline std::string_view ItemDefinitionCatalogJson;
}

namespace golarion::catalog
{
    inline nlohmann::json loadItemDefinitionCatalog()
    {
        return nlohmann::json::parse(embedded::ItemDefinitionCatalogJson);
    }
}
