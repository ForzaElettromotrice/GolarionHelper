#pragma once

#include "golarion/character/feat_definition_manager.hpp"
#include <vector>

namespace golarion
{
    // Owned by the sheet; must outlive every consumer of its definitions.
    class FeatCatalog final
    {
    public:
        void add(FeatDefinition definition);
        const FeatDefinition &get(std::string_view id) const;
        std::vector<FeatDefinition> customDefinitions() const;

    private:
        std::map<std::string, FeatDefinition> definitions_;
    };
}
