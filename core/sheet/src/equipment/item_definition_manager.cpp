#include "golarion/equipment/item_definition_manager.hpp"

#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/catalog/definition_path.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

namespace golarion
{
    ItemDefinitionManager::ItemDefinitionManager()
    {
        const auto index = catalog::readFile("item-library/index.json");
        if (!index.is_array()) throw std::invalid_argument("Item index must be an array");
        for (const auto &entry : index)
        {
            const auto id = entry.at("id").get<std::string>();
            catalog::definitionPath("item-library", id);
            if (!names_.emplace(id, entry.at("name").get<std::string>()).second)
                throw std::invalid_argument("Duplicate item index ID: " + id);
        }
    }

    std::vector<std::pair<std::string, std::string>> ItemDefinitionManager::catalogEntries()
    {
        return {names_.begin(), names_.end()};
    }

    bool ItemDefinitionManager::contains(std::string_view id)
    {
        return names_.contains(normalize(id));
    }

    ItemDefinitionManager &ItemDefinitionManager::instance()
    {
        static ItemDefinitionManager manager;
        return manager;
    }

    const ItemDefinition &ItemDefinitionManager::get(std::string_view id)
    {
        const auto key = normalize(id);
        const auto entry = names_.find(key);
        if (entry == names_.end()) throw std::invalid_argument("item definition is not registered: " + key);
        const std::scoped_lock lock(mutex_);
        if (const auto cached = definitions_.find(key); cached != definitions_.end()) return cached->second;
        auto definition = itemDefinitionFromJson(catalog::readFile(catalog::definitionPath("item-library", key)));
        if (definition.id != key || definition.name != entry->second)
            throw std::invalid_argument("Item definition disagrees with index: " + key);
        return definitions_.emplace(key, std::move(definition)).first->second;
    }
}
