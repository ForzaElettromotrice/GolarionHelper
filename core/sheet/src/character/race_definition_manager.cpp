#include "golarion/character/race_definition_manager.hpp"

#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/character/race_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>

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
        if (const auto cached = definitions_.find(normalizedId); cached != definitions_.end()) return cached->second;
        const auto name = names_.find(normalizedId);
        if (name == names_.end()) throw std::invalid_argument("race definition is not registered: " + normalizedId);
        auto definition = raceDefinitionFromJson(catalog::readFile("race-library/races/" + normalizedId + ".json"));
        if (definition.id != normalizedId || definition.name != name->second) throw std::invalid_argument("Race index mismatch");
        return definitions_.emplace(normalizedId, std::move(definition)).first->second;
    }

    std::vector<RaceSummaryView> RaceDefinitionManager::listRaces() const
    {
        std::vector<RaceSummaryView> result;
        result.reserve(names_.size());
        for (const auto &[id, name] : names_) result.push_back({id, name});
        return result;
    }

    std::vector<AlternateRacialFeatureView> RaceDefinitionManager::alternateFeaturesView(std::string_view raceDefinitionId)
    {
        const auto &definition = get(raceDefinitionId);
        std::vector<AlternateRacialFeatureView> result;
        result.reserve(definition.alternateFeatures.size());
        for (const auto &feature : definition.alternateFeatures)
            result.push_back({feature.id, feature.name, feature.description, feature.replaces});
        return result;
    }

    RaceDefinitionManager::RaceDefinitionManager()
    {
        const auto index = catalog::readFile("race-library/index.json");
        if (!index.is_array()) throw std::invalid_argument("Expected race index");
        for (const auto &entry : index)
        {
            const auto id = entry.at("id").get<std::string>();
            if (id.empty() || id.front() == '.' || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos ||
                !names_.emplace(id, entry.at("name").get<std::string>()).second) throw std::invalid_argument("Invalid or duplicate race index entry");
        }
    }
}
