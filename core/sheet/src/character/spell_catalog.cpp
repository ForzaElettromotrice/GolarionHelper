#include "golarion/character/spell_catalog.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/util/string_utils.hpp"
#include <nlohmann/json.hpp>

namespace golarion
{
    void SpellCatalog::add(SpellDefinition definition)
    {
        const auto id = normalize(definition.id);
        if (definitions_.contains(id) || SpellDefinitionManager::instance().contains(id))
            throw std::invalid_argument("spell definition is already registered: " + id);
        auto validated = spellDefinitionFromJson(spellDefinitionToJson(definition));
        definitions_.emplace(id, std::move(validated));
    }

    const SpellDefinition &SpellCatalog::get(std::string_view id) const
    {
        const auto key = normalize(id);
        const auto found = definitions_.find(key);
        if (found != definitions_.end()) return found->second;
        return SpellDefinitionManager::instance().get(key);
    }

    std::vector<SpellDefinition> SpellCatalog::customDefinitions() const
    {
        std::vector<SpellDefinition> result;
        for (const auto &[id, definition] : definitions_) result.push_back(definition);
        return result;
    }
}
