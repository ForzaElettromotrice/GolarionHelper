#include "golarion/equipment/item_definition_manager.hpp"
#include <limits>

#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>


namespace golarion
{
    bool ItemDefinitionManager::contains(std::string_view id)
    {
        const auto key = normalize(id);
        const std::scoped_lock lock(mutex_);
        if (!loaded_) loadCatalog();
        return definitions_.contains(key);
    }

    ItemDefinitionManager &ItemDefinitionManager::instance()
    {
        static ItemDefinitionManager manager;
        return manager;
    }

    const ItemDefinition &ItemDefinitionManager::get(std::string_view itemDefinitionId)
    {
        const std::string normalizedId = normalize(itemDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("item definition is not registered: " + normalizedId);
        }
        return definition->second;
    }

    void ItemDefinitionManager::loadCatalog()
    {
        try
        {
            const nlohmann::json catalog = nlohmann::json::parse(embedded::ItemDefinitionCatalogJson);
            if (!catalog.is_array())
            {
                throw std::invalid_argument("item definition catalog must be a JSON array");
            }

            std::map<std::string, ItemDefinition> loadedDefinitions;
            for (const nlohmann::json &entry : catalog)
            {
                if (!entry.is_object())
                {
                    throw std::invalid_argument("item definition catalog entries must be JSON objects");
                }

                ItemDefinition definition = itemDefinitionFromJson(entry);
                const std::string id = definition.id;
                if (!loadedDefinitions.emplace(id, std::move(definition)).second)
                {
                    throw std::invalid_argument("item definition is duplicated: " + id);
                }
            }

            definitions_ = std::move(loadedDefinitions);
            loaded_ = true;
        }
        catch (const nlohmann::json::exception &exception)
        {
            throw std::invalid_argument("invalid embedded item definition catalog: " + std::string(exception.what()));
        }
    }
}
