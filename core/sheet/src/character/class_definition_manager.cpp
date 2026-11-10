#include "golarion/character/class_definition_manager.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    const ClassDefinitionManager &ClassDefinitionManager::instance()
    {
        // Catalog content is intentionally deferred; never synthesize partial classes.
        static const ClassDefinitionManager manager({});
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
