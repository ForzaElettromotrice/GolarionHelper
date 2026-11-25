#include "golarion/character/feat_definition_manager.hpp"

#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/catalog/definition_path.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    std::vector<golarion::FeatRequirementView> requirementViews(const std::vector<golarion::FeatRequirementDefinition> &requirements)
    {
        std::vector<golarion::FeatRequirementView> result;
        for (const auto &requirement : requirements)
            result.push_back({requirement.description, !requirement.expression.has_value() && !requirement.feat.has_value()});
        return result;
    }

    std::vector<std::string> reminderNotes(const std::vector<golarion::EffectDefinition> &effects)
    {
        std::vector<std::string> result;
        for (const auto &effect : effects)
            if (const auto *reminder = std::get_if<golarion::ReminderEffectDefinition>(&effect)) result.push_back(reminder->message);
        return result;
    }
}

namespace golarion
{
    bool FeatDefinitionManager::contains(std::string_view id)
    {
        return entries_.contains(normalize(id));
    }

    FeatDefinitionManager &FeatDefinitionManager::instance()
    {
        static FeatDefinitionManager manager;
        return manager;
    }

    const FeatDefinition &FeatDefinitionManager::get(std::string_view featDefinitionId)
    {
        const auto key = normalize(featDefinitionId);
        const auto entry = entries_.find(key);
        if (entry == entries_.end()) throw std::invalid_argument("feat definition is not registered: " + key);
        const std::scoped_lock lock(mutex_);
        if (const auto cached = definitions_.find(key); cached != definitions_.end()) return cached->second;
        auto definition = featDefinitionFromJson(catalog::readFile(catalog::definitionPath("feat-library", key)));
        if (definition.id != key || definition.name != entry->second.name || definition.types != entry->second.types)
            throw std::invalid_argument("Feat definition disagrees with index: " + key);
        return definitions_.emplace(key, std::move(definition)).first->second;
    }

    std::vector<FeatDefinitionView> FeatDefinitionManager::toView()
    {
        std::vector<FeatDefinitionView> result;
        result.reserve(entries_.size());
        for (const auto &[id, entry] : entries_)
        {
            const auto &definition = get(id);
            std::vector<FeatCatalogChoiceView> choices;
            for (const auto &choice : definition.choices)
            {
                std::vector<FeatCatalogOptionView> options;
                for (const auto &option : choice.options)
                    options.push_back({option.id, option.name, requirementViews(option.requirements), reminderNotes(option.effects)});
                choices.push_back({choice.id, choice.prompt, choice.selectionCount, std::move(options)});
            }
            result.push_back({id, definition.name, definition.description, definition.types, requirementViews(definition.requirements),
                std::move(choices), definition.repeatability, reminderNotes(definition.effects)});
        }
        return result;
    }

    FeatDefinitionManager::FeatDefinitionManager()
    {
        const auto index = catalog::readFile("feat-library/index.json");
        if (!index.is_array()) throw std::invalid_argument("Feat index must be an array");
        for (const auto &entry : index)
        {
            const auto id = entry.at("id").get<std::string>();
            catalog::definitionPath("feat-library", id);
            CatalogEntry summary{id, entry.at("name").get<std::string>(), {}};
            for (const auto &name : entry.at("types").get<std::vector<std::string>>())
                summary.types.push_back(featTypeFromName(name));
            if (!entries_.emplace(id, std::move(summary)).second)
                throw std::invalid_argument("Duplicate feat index ID: " + id);
        }
    }

    std::vector<FeatDefinitionManager::CatalogEntry> FeatDefinitionManager::catalogEntries() const
    {
        std::vector<CatalogEntry> result;
        result.reserve(entries_.size());
        for (const auto &[id, entry] : entries_) result.push_back(entry);
        return result;
    }
}
