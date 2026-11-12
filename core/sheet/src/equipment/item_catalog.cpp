#include "golarion/equipment/item_catalog.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/util/string_utils.hpp"
#include <nlohmann/json.hpp>

namespace golarion
{
    void ItemCatalog::add(ItemDefinition definition)
    {
        const auto id = normalize(definition.id);
        if (definitions_.contains(id) || ItemDefinitionManager::instance().contains(id))
            throw std::invalid_argument("item definition is already registered: " + id);
        // Use the same validation and normalization as embedded definitions. This
        // also ensures every accepted custom definition can survive persistence.
        auto validated = itemDefinitionFromJson(itemDefinitionToJson(definition));
        definitions_.emplace(id, std::move(validated));
    }

    const ItemDefinition &ItemCatalog::get(std::string_view id) const
    {
        const auto key = normalize(id);
        const auto position = definitions_.find(key);
        if (position != definitions_.end()) return position->second;
        return ItemDefinitionManager::instance().get(key);
    }

    std::vector<ItemDefinition> ItemCatalog::customDefinitions() const
    {
        std::vector<ItemDefinition> result;
        for (const auto &[id, definition] : definitions_) result.push_back(definition);
        return result;
    }
}
