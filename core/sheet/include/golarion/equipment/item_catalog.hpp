#pragma once

#include "golarion/equipment/item_definition_manager.hpp"

namespace golarion
{
    // Per-sheet immutable definitions, with fallback to the embedded catalog.
    class ItemCatalog final
    {
    public:
        void add(ItemDefinition definition);
        const ItemDefinition &get(std::string_view id) const;
        std::vector<ItemDefinition> customDefinitions() const;

    private:
        std::map<std::string, ItemDefinition> definitions_;
    };
}
