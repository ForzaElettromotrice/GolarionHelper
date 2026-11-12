#include "golarion/character/feat_definition_manager.hpp"

#include "golarion/character/feat_definition_catalog.hpp"
#include "golarion/character/feat_definition_json.hpp"
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
    bool FeatDefinitionManager::contains(std::string_view id)
    {
        const auto key = normalize(id);
        const std::scoped_lock lock(mutex_);
        if (!loaded_) loadCatalog();
        return definitions_.contains(key);
    }

    FeatDefinitionManager &FeatDefinitionManager::instance()
    {
        static FeatDefinitionManager manager;
        return manager;
    }

    const FeatDefinition &FeatDefinitionManager::get(std::string_view featDefinitionId)
    {
        const std::string normalizedId = normalize(featDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("feat definition is not registered: " + normalizedId);
        }
        return definition->second;
    }

    void FeatDefinitionManager::loadCatalog()
    {
        try
        {
            const Json catalog = Json::parse(embedded::FeatDefinitionCatalogJson);
            if (!catalog.is_array())
            {
                throw std::invalid_argument("feat definition catalog must be a JSON array");
            }

            std::map<std::string, FeatDefinition> loadedDefinitions;
            for (const Json &entry : catalog)
            {
                FeatDefinition definition = featDefinitionFromJson(entry);
                const std::string id = definition.id;
                if (!loadedDefinitions.emplace(id, std::move(definition)).second)
                {
                    throw std::invalid_argument("feat definition is duplicated: " + id);
                }
            }

            definitions_ = std::move(loadedDefinitions);
            loaded_ = true;
        }
        catch (const nlohmann::json::exception &exception)
        {
            throw std::invalid_argument("invalid embedded feat definition catalog: " + std::string(exception.what()));
        }
    }
}
