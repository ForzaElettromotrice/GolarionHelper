#include "golarion/character/action.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    void normalizeTags(std::vector<std::string> &tags)
    {
        for (std::string &tag : tags)
        {
            tag = golarion::normalize(tag);
        }
        std::ranges::sort(tags);
        if (std::ranges::adjacent_find(tags) != tags.end())
        {
            throw std::invalid_argument("action tags must not contain duplicates");
        }
    }
}

namespace golarion
{
    std::string_view displayName(ActionCost cost)
    {
        switch (cost)
        {
            case ActionCost::Standard:
                return "Standard";
            case ActionCost::Move:
                return "Movimento";
            case ActionCost::FullRound:
                return "Round completo";
            case ActionCost::Swift:
                return "Veloce";
            case ActionCost::Immediate:
                return "Immediata";
            case ActionCost::Free:
                return "Gratuita";
            case ActionCost::ReplacesAttack:
                return "Sostituisce un attacco";
            case ActionCost::NotAnAction:
                return "Non è un'azione";
        }

        throw std::invalid_argument("unknown action cost");
    }

    ActionCategory::ActionCategory(ActionCategoryDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          name_(normalize(definition.name))
    {
    }

    Action::Action(ActionDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          categoryId_(normalize(definition.categoryId)),
          name_(normalize(definition.name)),
          description_(normalize(definition.description)),
          cost_(definition.cost),
          tags_(std::move(definition.tags))
    {
        normalizeTags(tags_);
    }

    ActionSelector::ActionSelector(ActionSelectorDefinition definition)
        : actionId_(std::move(definition.actionId)),
          categoryId_(std::move(definition.categoryId)),
          cost_(definition.cost),
          requiredTags_(std::move(definition.requiredTags))
    {
        if (actionId_.has_value())
        {
            actionId_ = normalize(*actionId_);
        }
        if (categoryId_.has_value())
        {
            categoryId_ = normalize(*categoryId_);
        }
        normalizeTags(requiredTags_);
    }

    bool ActionSelector::matches(const Action &action, ActionCost effectiveCost) const
    {
        if (actionId_.has_value() && *actionId_ != action.id_)
        {
            return false;
        }
        if (categoryId_.has_value() && *categoryId_ != action.categoryId_)
        {
            return false;
        }
        if (cost_.has_value() && *cost_ != effectiveCost)
        {
            return false;
        }
        return std::ranges::all_of(requiredTags_, [&action](const std::string &requiredTag)
        {
            return std::ranges::binary_search(action.tags_, requiredTag);
        });
    }

    ActionInhibition::ActionInhibition(ActionInhibitionDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          selector_(std::move(definition.selector)),
          reason_(normalize(definition.reason))
    {
    }

    ActionNote::ActionNote(ActionNoteDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          actionId_(normalize(definition.actionId)),
          text_(normalize(definition.text))
    {
    }

    ActionCostReplacement::ActionCostReplacement(ActionCostReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          actionId_(normalize(definition.actionId)),
          cost_(definition.cost)
    {
    }

    ActionManager::ActionManager(ResourceManager &resourceManager)
    {
        resourceManager.registerCollectionResource<ActionCategory>(ActionCategoriesResource, [this](ActionCategory category)
        {
            addCategory(std::move(category));
        }, [this](std::string_view categoryId)
        {
            removeCategory(categoryId);
        });
        resourceManager.registerCollectionResource<Action>(ActionGrantsResource, [this](Action action)
        {
            addAction(std::move(action));
        }, [this](std::string_view actionId)
        {
            removeAction(actionId);
        });
        resourceManager.registerCollectionResource<ActionInhibition>(ActionInhibitionsResource, [this](ActionInhibition inhibition)
        {
            addInhibition(std::move(inhibition));
        }, [this](std::string_view inhibitionId)
        {
            removeInhibition(inhibitionId);
        });
        resourceManager.registerCollectionResource<ActionNote>(ActionNotesResource, [this](ActionNote note)
        {
            addNote(std::move(note));
        }, [this](std::string_view noteId)
        {
            removeNote(noteId);
        });
        resourceManager.registerCollectionResource<ActionCostReplacement>(ActionCostReplacementsResource, [this](ActionCostReplacement replacement)
        {
            addCostReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeCostReplacement(replacementId);
        });

        registerCanonicalActions();
    }

    ActionsView ActionManager::toView() const
    {
        std::vector<ActionCategoryView> categoryViews;
        categoryViews.reserve(categories_.size());
        for (const auto &[categoryId, category] : categories_)
        {
            std::vector<ActionView> actionViews;
            for (const auto &[actionId, action] : actions_)
            {
                if (action.categoryId_ == categoryId)
                {
                    actionViews.push_back(*actionView(actionId));
                }
            }
            categoryViews.push_back(ActionCategoryView{
                .id = categoryId,
                .source = category.source_,
                .name = category.name_,
                .actions = std::move(actionViews)
            });
        }
        return ActionsView{.categories = std::move(categoryViews)};
    }

    std::optional<ActionView> ActionManager::actionView(std::string_view actionId) const
    {
        const std::string id = normalize(actionId);
        const auto action = actions_.find(id);
        if (action == actions_.end())
        {
            return std::nullopt;
        }

        const ActionCost resolvedCost = effectiveCost(action->second);
        std::vector<ActionInhibitionView> inhibitionViews;
        for (const auto &[inhibitionId, inhibition] : inhibitions_)
        {
            if (inhibition.selector_.matches(action->second, resolvedCost))
            {
                inhibitionViews.push_back(ActionInhibitionView{
                    .id = inhibitionId,
                    .source = inhibition.source_,
                    .reason = inhibition.reason_
                });
            }
        }
        std::vector<ActionNoteView> noteViews;
        for (const auto &[noteId, note] : notes_)
        {
            if (note.actionId_ == id)
            {
                noteViews.push_back(ActionNoteView{
                    .id = noteId,
                    .source = note.source_,
                    .text = note.text_
                });
            }
        }
        std::vector<ActionCostReplacementView> replacementViews;
        for (const auto &[replacementId, replacement] : costReplacements_)
        {
            if (replacement.actionId_ == id)
            {
                replacementViews.push_back(ActionCostReplacementView{
                    .id = replacementId,
                    .source = replacement.source_,
                    .cost = replacement.cost_
                });
            }
        }
        return ActionView{
            .id = id,
            .source = action->second.source_,
            .categoryId = action->second.categoryId_,
            .name = action->second.name_,
            .description = action->second.description_,
            .baseCost = action->second.cost_,
            .effectiveCost = resolvedCost,
            .tags = action->second.tags_,
            .notes = std::move(noteViews),
            .costReplacements = std::move(replacementViews),
            .usable = inhibitionViews.empty(),
            .inhibitions = std::move(inhibitionViews)
        };
    }

    void ActionManager::addCategory(ActionCategory category)
    {
        const std::string id = category.id_;
        if (!categories_.emplace(id, std::move(category)).second)
        {
            throw std::invalid_argument("action category is already registered: " + id);
        }
    }

    void ActionManager::removeCategory(std::string_view categoryId)
    {
        const std::string id = normalize(categoryId);
        if (!categories_.contains(id))
        {
            throw std::invalid_argument("action category is not registered: " + id);
        }
        if (std::ranges::any_of(actions_, [&id](const auto &entry)
        {
            return entry.second.categoryId_ == id;
        }))
        {
            throw std::invalid_argument("action category still contains actions: " + id);
        }
        categories_.erase(id);
    }

    void ActionManager::addAction(Action action)
    {
        const std::string id = action.id_;
        if (!categories_.contains(action.categoryId_))
        {
            throw std::invalid_argument("action category is not registered: " + action.categoryId_);
        }
        if (!actions_.emplace(id, std::move(action)).second)
        {
            throw std::invalid_argument("action is already registered: " + id);
        }
    }

    void ActionManager::removeAction(std::string_view actionId)
    {
        const std::string id = normalize(actionId);
        if (std::ranges::any_of(notes_, [&id](const auto &entry)
        {
            return entry.second.actionId_ == id;
        }))
        {
            throw std::invalid_argument("action still has notes: " + id);
        }
        if (std::ranges::any_of(costReplacements_, [&id](const auto &entry)
        {
            return entry.second.actionId_ == id;
        }))
        {
            throw std::invalid_argument("action still has cost replacements: " + id);
        }
        if (actions_.erase(id) == 0)
        {
            throw std::invalid_argument("action is not registered: " + id);
        }
    }

    void ActionManager::addInhibition(ActionInhibition inhibition)
    {
        const std::string id = inhibition.id_;
        if (!inhibitions_.emplace(id, std::move(inhibition)).second)
        {
            throw std::invalid_argument("action inhibition is already registered: " + id);
        }
    }

    void ActionManager::removeInhibition(std::string_view inhibitionId)
    {
        const std::string id = normalize(inhibitionId);
        if (inhibitions_.erase(id) == 0)
        {
            throw std::invalid_argument("action inhibition is not registered: " + id);
        }
    }

    void ActionManager::addNote(ActionNote note)
    {
        const std::string id = note.id_;
        if (!actions_.contains(note.actionId_))
        {
            throw std::invalid_argument("action is not registered: " + note.actionId_);
        }
        if (!notes_.emplace(id, std::move(note)).second)
        {
            throw std::invalid_argument("action note is already registered: " + id);
        }
    }

    void ActionManager::removeNote(std::string_view noteId)
    {
        const std::string id = normalize(noteId);
        if (notes_.erase(id) == 0)
        {
            throw std::invalid_argument("action note is not registered: " + id);
        }
    }

    void ActionManager::addCostReplacement(ActionCostReplacement replacement)
    {
        const std::string id = replacement.id_;
        if (!actions_.contains(replacement.actionId_))
        {
            throw std::invalid_argument("action is not registered: " + replacement.actionId_);
        }
        if (costReplacements_.contains(id))
        {
            throw std::invalid_argument("action cost replacement is already registered: " + id);
        }
        for (const auto &[existingId, existing] : costReplacements_)
        {
            static_cast<void>(existingId);
            if (existing.actionId_ == replacement.actionId_ && existing.cost_ != replacement.cost_)
            {
                throw std::invalid_argument("action has conflicting cost replacements: " + replacement.actionId_);
            }
        }
        costReplacements_.emplace(id, std::move(replacement));
    }

    void ActionManager::removeCostReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (costReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("action cost replacement is not registered: " + id);
        }
    }

    ActionCost ActionManager::effectiveCost(const Action &action) const
    {
        for (const auto &[replacementId, replacement] : costReplacements_)
        {
            static_cast<void>(replacementId);
            if (replacement.actionId_ == action.id_)
            {
                return replacement.cost_;
            }
        }
        return action.cost_;
    }
}
