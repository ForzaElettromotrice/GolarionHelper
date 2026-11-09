#pragma once

#include <string_view>

namespace golarion::embedded
{
    // Test-only catalog injection; the production catalog is unchanged.
    inline std::string_view ItemDefinitionCatalogJson;
}
