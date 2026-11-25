#include "golarion/character/class_definition_manager.hpp"
#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/character/archetype_definition_json.hpp"
#include "golarion/character/class_option_catalog_json.hpp"

namespace golarion
{
    const ClassDefinitionManager &ClassDefinitionManager::instance()
    {
        static const ClassDefinitionManager manager;
        return manager;
    }

    ClassDefinitionManager::ClassDefinitionManager() : diskBacked_(true)
    {
        const auto index = catalog::readFile("class-library/index.json");
        if (!index.is_array()) throw std::invalid_argument("Expected class index");
        for (const auto &entry : index)
            if (!names_.emplace(characterClassFromId(entry.at("type").get<std::string>()), entry.at("name").get<std::string>()).second)
                throw std::invalid_argument("Duplicate class index entry");
    }

    ClassDefinitionManager::ClassDefinitionManager(std::vector<ClassDefinition> definitions, std::vector<ArchetypeDefinition> archetypes)
    {
        for (auto &definition : definitions)
        {
            const auto type = definition.type;
            characterClassId(type);
            if (!names_.emplace(type, definition.name).second) throw std::invalid_argument("duplicate class definition");
            classes_.emplace(type, Record{std::move(definition), {}});
        }
        for (auto &archetype : archetypes)
        {
            const auto id = archetype.id;
            if (id.empty() || !classes_.contains(archetype.classType) || archetypes_.contains(id))
                throw std::invalid_argument("invalid or duplicate archetype definition");
            classes_.at(archetype.classType).archetypes.push_back({id, archetype.name});
            archetypes_.emplace(id, std::move(archetype));
        }
    }

    std::vector<ClassSummary> ClassDefinitionManager::listClasses() const
    {
        std::vector<ClassSummary> result;
        for (const auto &[type, name] : names_) result.push_back({type, name});
        return result;
    }

    const ClassDefinitionManager::Record &ClassDefinitionManager::record(CharacterClassType type) const
    {
        const std::scoped_lock lock(mutex_);
        if (const auto found = classes_.find(type); found != classes_.end()) return found->second;
        if (!diskBacked_ || !names_.contains(type)) throw std::invalid_argument("class definition is not registered");
        const auto file = catalog::readFile("class-library/classes/" + std::string(characterClassId(type)) + ".json");
        using Json = nlohmann::json;
        const auto meta = catalog::readFile("class-library/meta/" + std::string(characterClassId(type)) + ".json");
        auto sets = Json::array();
        for (const auto &key : meta.at("optionSets"))
        {
            const auto id = key.get<std::string>();
            if (id.empty() || id.front() == '.' || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
                throw std::invalid_argument("Invalid class option set ID");
            sets.push_back(catalog::readFile("class-library/options/" + id + ".json"));
        }
        auto definitions = classDefinitionsWithOptionsFromJson(Json::array({file}), Json::array({Json{{"bindings", file.at("bindings")}, {"optionSets", std::move(sets)}}}));
        Record result{std::move(definitions.at(0)), {}};
        if (result.definition.type != type || result.definition.name != names_.at(type)) throw std::invalid_argument("Class index mismatch");
        for (const auto &entry : meta.at("archetypes"))
            result.archetypes.push_back({entry.at("id").get<std::string>(), entry.at("name").get<std::string>()});
        return classes_.emplace(type, std::move(result)).first->second;
    }

    const ClassDefinition &ClassDefinitionManager::get(CharacterClassType type) const { return record(type).definition; }

    std::vector<ArchetypeSummary> ClassDefinitionManager::listArchetypes(CharacterClassType type) const { return record(type).archetypes; }

    const ArchetypeDefinition &ClassDefinitionManager::getArchetype(const std::string &id) const
    {
        const std::scoped_lock lock(mutex_);
        if (const auto found = archetypes_.find(id); found != archetypes_.end()) return found->second;
        const auto separator = id.find(".archetype.");
        if (!diskBacked_ || separator == std::string::npos || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
            throw std::invalid_argument("archetype definition is not registered");
        const auto owner = characterClassFromId(id.substr(0, separator));
        if (!names_.contains(owner)) throw std::invalid_argument("unknown archetype class");
        auto definition = archetypeDefinitionFromJson(catalog::readFile("class-library/archetypes/" + id + ".json"));
        if (definition.id != id || definition.classType != owner) throw std::invalid_argument("Archetype ID mismatch");
        return archetypes_.emplace(id, std::move(definition)).first->second;
    }
}
