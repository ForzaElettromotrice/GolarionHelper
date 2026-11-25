#include "golarion/character/spell_definition_manager.hpp"

#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/catalog/definition_path.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

namespace golarion
{
    SpellDefinitionManager::SpellDefinitionManager()
    {
        const auto index = catalog::readFile("spell-library/index.json");
        if (!index.is_array()) throw std::invalid_argument("Spell index must be an array");
        for (const auto &entry : index)
        {
            const auto id = entry.at("id").get<std::string>();
            catalog::definitionPath("spell-library", id);
            CatalogEntry summary{id, entry.at("name").get<std::string>(), {}};
            const auto &levels = entry.at("levels");
            if (!levels.is_object()) throw std::invalid_argument("Invalid spell index levels: " + id);
            for (const auto &[key, value] : levels.items())
            {
                if (!value.is_number_integer() || value < 0 || value > 9)
                    throw std::invalid_argument("Invalid spell index level: " + id);
                summary.levels.emplace(characterClassFromId(key), value.get<int>());
            }
            if (!entries_.emplace(id, std::move(summary)).second)
                throw std::invalid_argument("Duplicate spell index ID: " + id);
        }
    }

    SpellDefinitionManager &SpellDefinitionManager::instance()
    {
        static SpellDefinitionManager manager;
        return manager;
    }

    std::vector<SpellDefinitionManager::CatalogEntry> SpellDefinitionManager::catalogEntries()
    {
        std::vector<CatalogEntry> result;
        result.reserve(entries_.size());
        for (const auto &[id, entry] : entries_) result.push_back(entry);
        return result;
    }

    bool SpellDefinitionManager::contains(std::string_view id)
    {
        return entries_.contains(normalize(id));
    }

    const SpellDefinition &SpellDefinitionManager::get(std::string_view id)
    {
        const auto key = normalize(id);
        const auto entry = entries_.find(key);
        if (entry == entries_.end()) throw std::invalid_argument("spell definition is not registered: " + key);
        const std::scoped_lock lock(mutex_);
        if (const auto cached = definitions_.find(key); cached != definitions_.end()) return cached->second;
        auto definition = spellDefinitionFromJson(catalog::readFile(catalog::definitionPath("spell-library", key)));
        if (definition.id != key || definition.name != entry->second.name || definition.spellLevelByClass != entry->second.levels)
            throw std::invalid_argument("Spell definition disagrees with index: " + key);
        return definitions_.emplace(key, std::move(definition)).first->second;
    }
}
