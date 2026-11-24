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

    std::vector<FeatDefinitionView> FeatDefinitionManager::toView()
    {
        const std::scoped_lock lock(mutex_);
        if (!loaded_) loadCatalog();
        std::vector<FeatDefinitionView> result;
        result.reserve(definitions_.size());
        for (const auto &[id, definition] : definitions_)
        {
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

    void FeatDefinitionManager::loadCatalog()
    {
        try
        {
            const Json catalog = catalog::loadFeatDefinitionCatalog();
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
            throw std::invalid_argument("invalid feat definition catalog: " + std::string(exception.what()));
        }
    }
}
