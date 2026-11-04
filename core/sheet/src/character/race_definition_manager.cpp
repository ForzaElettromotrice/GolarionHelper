#include "golarion/character/race_definition_manager.hpp"

#include "golarion/character/race_definition_catalog.hpp"
#include "golarion/character/race_definition_json.hpp"
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
    RaceDefinitionManager &RaceDefinitionManager::instance()
    {
        static RaceDefinitionManager manager;
        return manager;
    }

    const RaceDefinition &RaceDefinitionManager::get(std::string_view raceDefinitionId)
    {
        const std::string normalizedId = normalize(raceDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("race definition is not registered: " + normalizedId);
        }
        return definition->second;
    }

    void RaceDefinitionManager::loadCatalog()
    {
        try
        {
            const Json catalog = Json::parse(embedded::RaceDefinitionCatalogJson);
            if (!catalog.is_array())
            {
                throw std::invalid_argument("race definition catalog must be a JSON array");
            }

            std::map<std::string, RaceDefinition> loadedDefinitions;
            for (const Json &entry : catalog)
            {
                RaceDefinition definition = raceDefinitionFromJson(entry);
                const std::string id = definition.id;
                if (!loadedDefinitions.emplace(id, std::move(definition)).second)
                {
                    throw std::invalid_argument("race definition is duplicated: " + id);
                }
            }

            definitions_ = std::move(loadedDefinitions);
            loaded_ = true;
        }
        catch (const nlohmann::json::exception &exception)
        {
            throw std::invalid_argument("invalid embedded race definition catalog: " + std::string(exception.what()));
        }
    }
}
