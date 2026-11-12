#include "golarion/character/feat_catalog.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "golarion/util/string_utils.hpp"
#include <nlohmann/json.hpp>

namespace golarion
{
    void FeatCatalog::add(FeatDefinition definition)
    {
        const auto id = normalize(definition.id);
        if (definitions_.contains(id) || FeatDefinitionManager::instance().contains(id))
            throw std::invalid_argument("feat definition is already registered: " + id);
        auto validated = featDefinitionFromJson(featDefinitionToJson(definition));
        definitions_.emplace(id, std::move(validated));
    }

    const FeatDefinition &FeatCatalog::get(std::string_view id) const
    {
        const auto key = normalize(id);
        const auto found = definitions_.find(key);
        if (found != definitions_.end()) return found->second;
        return FeatDefinitionManager::instance().get(key);
    }

    std::vector<FeatDefinition> FeatCatalog::customDefinitions() const
    {
        std::vector<FeatDefinition> result;
        for (const auto &[id, definition] : definitions_) result.push_back(definition);
        return result;
    }
}
