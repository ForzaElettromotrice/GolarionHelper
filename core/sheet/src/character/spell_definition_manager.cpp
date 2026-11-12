#include "golarion/character/spell_definition_manager.hpp"

#include "golarion/character/spell_definition_catalog.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;
}

namespace golarion
{
    bool SpellDefinitionManager::contains(std::string_view id)
    {
        const auto key = normalize(id);
        const std::scoped_lock lock(mutex_);
        if (!loaded_) loadCatalog();
        return definitions_.contains(key);
    }

    SpellDefinitionManager &SpellDefinitionManager::instance()
    {
        static SpellDefinitionManager manager;
        return manager;
    }

    const SpellDefinition &SpellDefinitionManager::get(std::string_view spellDefinitionId)
    {
        const std::string normalizedId = normalize(spellDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("spell definition is not registered: " + normalizedId);
        }
        return definition->second;
    }

    void SpellDefinitionManager::loadCatalog()
    {
        try
        {
            const Json catalog = Json::parse(embedded::SpellDefinitionCatalogJson);
            if (!catalog.is_array())
            {
                throw std::invalid_argument("spell definition catalog must be a JSON array");
            }

            std::map<std::string, SpellDefinition> loadedDefinitions;
            for (const Json &entry : catalog)
            {
                SpellDefinition definition = spellDefinitionFromJson(entry);
                const std::string id = definition.id;
                if (!loadedDefinitions.emplace(id, std::move(definition)).second)
                {
                    throw std::invalid_argument("spell definition is duplicated: " + id);
                }
            }

            definitions_ = std::move(loadedDefinitions);
            loaded_ = true;
        }
        catch (const nlohmann::json::exception &exception)
        {
            throw std::invalid_argument("invalid embedded spell definition catalog: " + std::string(exception.what()));
        }
    }
}
