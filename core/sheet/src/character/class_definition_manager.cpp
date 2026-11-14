#include "golarion/character/class_definition_manager.hpp"
#include "golarion/character/class_definition_catalog.hpp"
#include "golarion/character/class_definition_json.hpp"
#include "golarion/character/archetype_definition_catalog.hpp"
#include "golarion/character/archetype_definition_json.hpp"
#include "golarion/character/class_option_catalog.hpp"
#include "golarion/character/class_option_catalog_json.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>

namespace golarion
{
    const ClassDefinitionManager &ClassDefinitionManager::instance()
    {
        static const ClassDefinitionManager manager = []
        {
            const auto catalog = nlohmann::json::parse(embedded::ClassDefinitionCatalogJson);
            if (!catalog.is_array()) throw std::invalid_argument("class catalog must be a JSON array");
            auto definitions = classDefinitionsWithOptionsFromJson(catalog, nlohmann::json::parse(embedded::ClassOptionCatalogJson));
            const auto archetypeCatalog = nlohmann::json::parse(embedded::ArchetypeDefinitionCatalogJson);
            if (!archetypeCatalog.is_array()) throw std::invalid_argument("archetype catalog must be a JSON array");
            std::vector<ArchetypeDefinition> archetypes;
            archetypes.reserve(archetypeCatalog.size());
            for (const auto &entry : archetypeCatalog) archetypes.push_back(archetypeDefinitionFromJson(entry));
            return ClassDefinitionManager(std::move(definitions), std::move(archetypes));
        }();
        return manager;
    }

    ClassDefinitionManager::ClassDefinitionManager(std::vector<ClassDefinition> definitions, std::vector<ArchetypeDefinition> archetypes)
    {
        for (auto &definition : definitions)
        {
            const auto type = definition.type;
            characterClassId(type);
            if (!definitions_.emplace(type, std::move(definition)).second)
                throw std::invalid_argument("duplicate class definition");
        }
        for (auto &archetype : archetypes)
        {
            const auto id = archetype.id;
            if (id.empty() || !definitions_.contains(archetype.classType) || !archetypes_.emplace(id, std::move(archetype)).second)
                throw std::invalid_argument("invalid or duplicate archetype definition");
        }
    }

    const ArchetypeDefinition &ClassDefinitionManager::getArchetype(const std::string &id) const
    {
        const auto found = archetypes_.find(id);
        if (found == archetypes_.end()) throw std::invalid_argument("archetype definition is not registered");
        return found->second;
    }

    const ClassDefinition &ClassDefinitionManager::get(CharacterClassType type) const
    {
        const auto found = definitions_.find(type);
        if (found == definitions_.end()) throw std::invalid_argument("class definition is not registered");
        return found->second;
    }
}
