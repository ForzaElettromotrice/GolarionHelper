#include "golarion/character/condition.hpp"

#include "golarion/data/condition_manager_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/condition_manager_view.hpp"

#include <algorithm>
#include <exception>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    int checkedSeverity(long long severity)
    {
        if (severity > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("condition severity is out of range");
        }
        return static_cast<int>(severity);
    }

    std::string effectInstanceId(std::string_view conditionId, std::string_view stageId, std::string_view effectId, const std::optional<std::string> &entryId)
    {
        const auto appendPart = [](std::string &result, std::string_view part)
        {
            result += std::to_string(part.size()) + ":" + std::string(part);
        };

        std::string result = "condition.effect.";
        appendPart(result, conditionId);
        appendPart(result, stageId);
        appendPart(result, effectId);
        if (entryId.has_value())
        {
            appendPart(result, *entryId);
        }
        return result;
    }
}

namespace golarion
{
    std::string_view displayName(ConditionStackingMode mode)
    {
        switch (mode)
        {
            case ConditionStackingMode::Shared:
                return "Condivisa";
            case ConditionStackingMode::Escalating:
                return "Progressiva";
            case ConditionStackingMode::PerEntry:
                return "Per fonte";
        }

        throw std::invalid_argument("unknown condition stacking mode");
    }

    std::string_view displayName(ConditionEntryOrigin origin)
    {
        switch (origin)
        {
            case ConditionEntryOrigin::Manual:
                return "Manuale";
            case ConditionEntryOrigin::SourceControlled:
                return "Controllata dalla fonte";
            case ConditionEntryOrigin::Derived:
                return "Derivata";
        }

        throw std::invalid_argument("unknown condition entry origin");
    }

    Condition::Condition(ConditionDefinition definition)
        : id_(normalize(definition.id)),
          name_(normalize(definition.name)),
          stackingMode_(definition.stackingMode),
          stages_(std::move(definition.stages)),
          entryParameterName_(std::move(definition.entryParameterName))
    {
        if (entryParameterName_.has_value())
        {
            entryParameterName_ = normalize(*entryParameterName_);
        }
        if (stages_.empty())
        {
            throw std::invalid_argument("condition requires at least one stage");
        }
        if (stages_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
            throw std::invalid_argument("condition has too many stages");
        }

        std::vector<std::string> stageIds;
        stageIds.reserve(stages_.size());
        for (ConditionStageDefinition &stage : stages_)
        {
            stage.id = normalize(stage.id);
            stage.name = normalize(stage.name);
            stageIds.push_back(stage.id);

            std::vector<std::string> derivedConditionIds;
            derivedConditionIds.reserve(stage.derivedConditions.size());
            for (DerivedConditionDefinition &derivedCondition : stage.derivedConditions)
            {
                derivedCondition.conditionId = normalize(derivedCondition.conditionId);
                if (derivedCondition.severity <= 0)
                {
                    throw std::invalid_argument("derived condition severity must be positive");
                }
                derivedConditionIds.push_back(derivedCondition.conditionId);
            }
            std::ranges::sort(derivedConditionIds);
            if (std::ranges::adjacent_find(derivedConditionIds) != derivedConditionIds.end())
            {
                throw std::invalid_argument("derived conditions must be unique within a stage");
            }

            std::vector<std::string> effectIds;
            effectIds.reserve(stage.effects.size());
            for (ConditionEffectDefinition &effect : stage.effects)
            {
                effect.id = normalize(effect.id);
                effect.description = normalize(effect.description);
                if (!effect.apply)
                {
                    throw std::invalid_argument("condition effect apply callback must not be empty");
                }
                effectIds.push_back(effect.id);
            }
            std::ranges::sort(effectIds);
            if (std::ranges::adjacent_find(effectIds) != effectIds.end())
            {
                throw std::invalid_argument("condition effect IDs must be unique within a stage");
            }
        }
        std::ranges::sort(stageIds);
        if (std::ranges::adjacent_find(stageIds) != stageIds.end())
        {
            throw std::invalid_argument("condition stage IDs must be unique");
        }
    }

    ConditionEntry::ConditionEntry(ConditionEntryDefinition definition)
        : id_(normalize(definition.id)),
          conditionId_(normalize(definition.conditionId)),
          source_(normalize(definition.source)),
          severity_(definition.severity),
          contributesToEscalation_(definition.contributesToEscalation),
          stackingGroup_(std::move(definition.stackingGroup)),
          parameter_(std::move(definition.parameter))
    {
        if (severity_ <= 0)
        {
            throw std::invalid_argument("condition entry severity must be positive");
        }
        if (stackingGroup_.has_value())
        {
            stackingGroup_ = normalize(*stackingGroup_);
        }
        if (parameter_.has_value())
        {
            parameter_ = normalize(*parameter_);
        }
    }

    ConditionManager::ConditionManager(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager.registerCollectionResource<ConditionEntry>(ConditionEntriesResource, [this](ConditionEntry entry)
        {
            addSourceControlledEntry(std::move(entry));
        }, [this](std::string_view entryId)
        {
            removeSourceControlledEntry(entryId);
        });
    }

    ConditionManager::~ConditionManager()
    {
        for (auto iterator = appliedEffects_.rbegin(); iterator != appliedEffects_.rend(); ++iterator)
        {
            iterator->second.cleanup();
        }
        resourceManager_.unregisterCollectionResource(ConditionEntriesResource);
    }

    void ConditionManager::registerDefinition(Condition condition)
    {
        const std::string conditionId = condition.id_;
        if (!definitions_.emplace(conditionId, std::move(condition)).second)
        {
            throw std::invalid_argument("condition definition is already registered: " + conditionId);
        }

        try
        {
            validateDerivedConditionGraph();
            commitEntries(entries_);
        }
        catch (...)
        {
            definitions_.erase(conditionId);
            throw;
        }
    }

    void ConditionManager::addManualEntry(ConditionEntry entry)
    {
        addEntry(std::move(entry), ConditionEntryOrigin::Manual);
    }

    void ConditionManager::removeManualEntry(std::string_view entryId)
    {
        const std::string normalizedEntryId = normalize(entryId);
        const StoredEntry &stored = storedEntry(normalizedEntryId);
        if (stored.origin != ConditionEntryOrigin::Manual)
        {
            throw std::invalid_argument("condition entry is not manually controlled: " + normalizedEntryId);
        }
        std::map<std::string, StoredEntry> reconciledEntries = entries_;
        reconciledEntries.erase(normalizedEntryId);
        commitEntries(std::move(reconciledEntries));
    }

    bool ConditionManager::isActive(std::string_view conditionId) const
    {
        return entryCount(conditionId) != 0;
    }

    std::size_t ConditionManager::entryCount(std::string_view conditionId) const
    {
        const std::string normalizedConditionId = normalize(conditionId);
        static_cast<void>(definition(normalizedConditionId));
        return static_cast<std::size_t>(std::ranges::count_if(entries_, [&normalizedConditionId](const auto &pair)
        {
            return pair.second.entry.conditionId_ == normalizedConditionId;
        }));
    }

    int ConditionManager::effectiveSeverity(std::string_view conditionId) const
    {
        return effectiveSeverity(definition(conditionId), entries_);
    }

    int ConditionManager::effectiveSeverity(const Condition &condition, const std::map<std::string, StoredEntry> &entries) const
    {
        const bool active = std::ranges::any_of(entries, [&condition](const auto &pair)
        {
            return pair.second.entry.conditionId_ == condition.id_;
        });
        if (!active)
        {
            return 0;
        }

        if (condition.stackingMode_ == ConditionStackingMode::Shared)
        {
            return 1;
        }

        long long totalSeverity = 0;
        if (condition.stackingMode_ == ConditionStackingMode::PerEntry)
        {
            for (const auto &[entryId, stored] : entries)
            {
                static_cast<void>(entryId);
                if (stored.entry.conditionId_ == condition.id_)
                {
                    totalSeverity += stored.entry.severity_;
                }
            }
            return checkedSeverity(totalSeverity);
        }

        if (condition.stackingMode_ != ConditionStackingMode::Escalating)
        {
            throw std::invalid_argument("unknown condition stacking mode");
        }

        const int maximumSeverity = static_cast<int>(condition.stages_.size());
        int minimumSeverity = 0;
        std::map<std::string, int> groupedSeverities;
        for (const auto &[entryId, stored] : entries)
        {
            static_cast<void>(entryId);
            const ConditionEntry &entry = stored.entry;
            if (entry.conditionId_ != condition.id_)
            {
                continue;
            }
            if (!entry.contributesToEscalation_)
            {
                minimumSeverity = std::min(maximumSeverity, std::max(minimumSeverity, entry.severity_));
                continue;
            }
            if (!entry.stackingGroup_.has_value())
            {
                totalSeverity = std::min<long long>(maximumSeverity, totalSeverity + entry.severity_);
                continue;
            }

            int &groupSeverity = groupedSeverities[*entry.stackingGroup_];
            groupSeverity = std::max(groupSeverity, entry.severity_);
        }

        for (const auto &[group, severity] : groupedSeverities)
        {
            static_cast<void>(group);
            totalSeverity = std::min<long long>(maximumSeverity, totalSeverity + severity);
        }
        return std::max(static_cast<int>(totalSeverity), minimumSeverity);
    }

    ConditionEntryOrigin ConditionManager::entryOrigin(std::string_view entryId) const
    {
        return storedEntry(entryId).origin;
    }

    ConditionManagerView ConditionManager::toView() const
    {
        const std::map<std::string, DesiredEffect> desired = desiredEffects(entries_);
        std::vector<ConditionView> conditionViews;
        conditionViews.reserve(definitions_.size());

        for (const auto &[conditionId, condition] : definitions_)
        {
            static_cast<void>(conditionId);
            const int resolvedSeverity = effectiveSeverity(condition, entries_);
            std::vector<ConditionEntryView> entryViews;
            for (const auto &[entryId, stored] : entries_)
            {
                static_cast<void>(entryId);
                if (stored.entry.conditionId_ != condition.id_)
                {
                    continue;
                }
                entryViews.push_back(ConditionEntryView{
                    .id = stored.entry.id_,
                    .source = stored.entry.source_,
                    .severity = stored.entry.severity_,
                    .contributesToEscalation = stored.entry.contributesToEscalation_,
                    .stackingGroup = stored.entry.stackingGroup_,
                    .origin = stored.origin,
                    .parameter = stored.entry.parameter_
                });
            }

            std::vector<ConditionStageView> stageViews;
            stageViews.reserve(condition.stages_.size());
            for (std::size_t stageIndex = 0; stageIndex < condition.stages_.size(); ++stageIndex)
            {
                const ConditionStageDefinition &stage = condition.stages_[stageIndex];
                const int stageSeverity = static_cast<int>(stageIndex) + 1;
                bool active = resolvedSeverity >= stageSeverity;
                if (condition.stackingMode_ == ConditionStackingMode::PerEntry)
                {
                    active = std::ranges::any_of(entries_, [&condition, stageSeverity](const auto &pair)
                    {
                        return pair.second.entry.conditionId_ == condition.id_ && pair.second.entry.severity_ >= stageSeverity;
                    });
                }

                std::vector<DerivedConditionView> derivedConditionViews;
                derivedConditionViews.reserve(stage.derivedConditions.size());
                for (const DerivedConditionDefinition &derivedCondition : stage.derivedConditions)
                {
                    derivedConditionViews.push_back(DerivedConditionView{
                        .conditionId = derivedCondition.conditionId,
                        .severity = derivedCondition.severity
                    });
                }

                std::vector<ConditionEffectView> effectViews;
                effectViews.reserve(stage.effects.size());
                for (const ConditionEffectDefinition &effect : stage.effects)
                {
                    std::vector<ConditionEffectInstanceView> instances;
                    for (const auto &[instanceId, desiredEffect] : desired)
                    {
                        if (desiredEffect.definition != &effect)
                        {
                            continue;
                        }
                        instances.push_back(ConditionEffectInstanceView{
                            .instanceId = instanceId,
                            .entryId = desiredEffect.context.entryId,
                            .source = desiredEffect.context.source
                        });
                    }
                    effectViews.push_back(ConditionEffectView{
                        .id = effect.id,
                        .description = effect.description,
                        .activeInstances = std::move(instances)
                    });
                }

                stageViews.push_back(ConditionStageView{
                    .id = stage.id,
                    .name = stage.name,
                    .severity = stageSeverity,
                    .active = active,
                    .derivedConditions = std::move(derivedConditionViews),
                    .effects = std::move(effectViews)
                });
            }

            conditionViews.push_back(ConditionView{
                .id = condition.id_,
                .name = condition.name_,
                .stackingMode = condition.stackingMode_,
                .effectiveSeverity = resolvedSeverity,
                .stages = std::move(stageViews),
                .entries = std::move(entryViews),
                .entryParameterName = condition.entryParameterName_
            });
        }
        return ConditionManagerView{.conditions = std::move(conditionViews)};
    }

    ConditionManagerSaveData ConditionManager::toSaveData() const
    {
        std::vector<ConditionEntrySaveData> manualEntries;
        for (const auto &[entryId, stored] : entries_)
        {
            static_cast<void>(entryId);
            if (stored.origin != ConditionEntryOrigin::Manual)
            {
                continue;
            }
            manualEntries.push_back(ConditionEntrySaveData{
                .id = stored.entry.id_,
                .conditionId = stored.entry.conditionId_,
                .source = stored.entry.source_,
                .severity = stored.entry.severity_,
                .contributesToEscalation = stored.entry.contributesToEscalation_,
                .stackingGroup = stored.entry.stackingGroup_,
                .parameter = stored.entry.parameter_
            });
        }
        return ConditionManagerSaveData{.manualEntries = std::move(manualEntries)};
    }

    void ConditionManager::load(const ConditionManagerSaveData &data)
    {
        std::map<std::string, StoredEntry> loadedEntries = entries_;
        for (const ConditionEntrySaveData &entryData : data.manualEntries)
        {
            ConditionEntry entry(ConditionEntryDefinition{
                .id = entryData.id,
                .conditionId = entryData.conditionId,
                .source = entryData.source,
                .severity = entryData.severity,
                .contributesToEscalation = entryData.contributesToEscalation,
                .stackingGroup = entryData.stackingGroup,
                .parameter = entryData.parameter
            });
            validateEntry(entry);
            const std::string entryId = entry.id_;
            if (!loadedEntries.emplace(entryId, StoredEntry{.entry = std::move(entry), .origin = ConditionEntryOrigin::Manual}).second)
            {
                throw std::invalid_argument("condition entry is duplicated in save data: " + entryId);
            }
        }
        commitEntries(std::move(loadedEntries));
    }

    void ConditionManager::addEntry(ConditionEntry entry, ConditionEntryOrigin origin)
    {
        validateEntry(entry);
        const std::string entryId = entry.id_;
        std::map<std::string, StoredEntry> reconciledEntries = entries_;
        if (!reconciledEntries.emplace(entryId, StoredEntry{.entry = std::move(entry), .origin = origin}).second)
        {
            throw std::invalid_argument("condition entry is already registered: " + entryId);
        }
        commitEntries(std::move(reconciledEntries));
    }

    void ConditionManager::validateEntry(const ConditionEntry &entry) const
    {
        const Condition &condition = definition(entry.conditionId_);
        if (condition.entryParameterName_.has_value() != entry.parameter_.has_value())
        {
            if (condition.entryParameterName_.has_value())
            {
                throw std::invalid_argument("condition entry requires parameter: " + *condition.entryParameterName_);
            }
            throw std::invalid_argument("condition entry does not accept a parameter: " + entry.conditionId_);
        }
    }

    void ConditionManager::addSourceControlledEntry(ConditionEntry entry)
    {
        addEntry(std::move(entry), ConditionEntryOrigin::SourceControlled);
    }

    void ConditionManager::removeSourceControlledEntry(std::string_view entryId)
    {
        const std::string normalizedEntryId = normalize(entryId);
        const StoredEntry &stored = storedEntry(normalizedEntryId);
        if (stored.origin != ConditionEntryOrigin::SourceControlled)
        {
            throw std::invalid_argument("condition entry is not controlled by a source: " + normalizedEntryId);
        }
        std::map<std::string, StoredEntry> reconciledEntries = entries_;
        reconciledEntries.erase(normalizedEntryId);
        commitEntries(std::move(reconciledEntries));
    }

    void ConditionManager::reconcileDerivedEntries(std::map<std::string, StoredEntry> &entries) const
    {
        std::erase_if(entries, [](const auto &pair)
        {
            return pair.second.origin == ConditionEntryOrigin::Derived;
        });

        bool addedEntry;
        do
        {
            addedEntry = false;
            for (const auto &[conditionId, condition] : definitions_)
            {
                static_cast<void>(conditionId);
                const int activeStageCount = std::min(effectiveSeverity(condition, entries), static_cast<int>(condition.stages_.size()));
                for (int stageIndex = 0; stageIndex < activeStageCount; ++stageIndex)
                {
                    const ConditionStageDefinition &stage = condition.stages_[static_cast<std::size_t>(stageIndex)];
                    for (const DerivedConditionDefinition &derivedCondition : stage.derivedConditions)
                    {
                        static_cast<void>(definition(derivedCondition.conditionId));
                        const std::string entryId = "condition.derived." + condition.id_ + "." + stage.id + "." + derivedCondition.conditionId;
                        const auto existing = entries.find(entryId);
                        if (existing != entries.end())
                        {
                            if (existing->second.origin != ConditionEntryOrigin::Derived)
                            {
                                throw std::invalid_argument("derived condition entry ID conflicts with another entry: " + entryId);
                            }
                            continue;
                        }

                        ConditionEntry entry(ConditionEntryDefinition{
                            .id = entryId,
                            .conditionId = derivedCondition.conditionId,
                            .source = condition.name_ + ": " + stage.name,
                            .severity = derivedCondition.severity
                        });
                        validateEntry(entry);
                        entries.emplace(entryId, StoredEntry{
                            .entry = std::move(entry),
                            .origin = ConditionEntryOrigin::Derived
                        });
                        addedEntry = true;
                    }
                }
            }
        }
        while (addedEntry);
    }

    void ConditionManager::commitEntries(std::map<std::string, StoredEntry> entries)
    {
        reconcileDerivedEntries(entries);
        transitionEffects(desiredEffects(entries));
        entries_ = std::move(entries);
    }

    std::map<std::string, ConditionManager::DesiredEffect> ConditionManager::desiredEffects(const std::map<std::string, StoredEntry> &entries) const
    {
        std::map<std::string, DesiredEffect> result;
        const auto addStageEffects = [&result](const Condition &condition, const ConditionStageDefinition &stage, int stageSeverity, const StoredEntry *stored)
        {
            const std::optional<std::string> entryId = stored == nullptr ? std::nullopt : std::optional<std::string>(stored->entry.id_);
            for (const ConditionEffectDefinition &effect : stage.effects)
            {
                ConditionEffectContext context{
                    .instanceId = effectInstanceId(condition.id_, stage.id, effect.id, entryId),
                    .conditionId = condition.id_,
                    .stageId = stage.id,
                    .stageSeverity = stageSeverity,
                    .entryId = entryId,
                    .source = stored == nullptr ? std::nullopt : std::optional<std::string>(stored->entry.source_),
                    .parameter = stored == nullptr ? std::nullopt : stored->entry.parameter_
                };
                const std::string instanceId = context.instanceId;
                if (!result.emplace(instanceId, DesiredEffect{.definition = &effect, .context = std::move(context)}).second)
                {
                    throw std::invalid_argument("condition effect instance is duplicated: " + instanceId);
                }
            }
        };

        for (const auto &[conditionId, condition] : definitions_)
        {
            static_cast<void>(conditionId);
            if (condition.stackingMode_ == ConditionStackingMode::PerEntry)
            {
                for (const auto &[entryId, stored] : entries)
                {
                    static_cast<void>(entryId);
                    if (stored.entry.conditionId_ != condition.id_)
                    {
                        continue;
                    }
                    const int activeStageCount = std::min(stored.entry.severity_, static_cast<int>(condition.stages_.size()));
                    for (int stageIndex = 0; stageIndex < activeStageCount; ++stageIndex)
                    {
                        addStageEffects(condition, condition.stages_[static_cast<std::size_t>(stageIndex)], stageIndex + 1, &stored);
                    }
                }
                continue;
            }

            const int activeStageCount = std::min(effectiveSeverity(condition, entries), static_cast<int>(condition.stages_.size()));
            for (int stageIndex = 0; stageIndex < activeStageCount; ++stageIndex)
            {
                addStageEffects(condition, condition.stages_[static_cast<std::size_t>(stageIndex)], stageIndex + 1, nullptr);
            }
        }
        return result;
    }

    void ConditionManager::transitionEffects(const std::map<std::string, DesiredEffect> &desiredEffects)
    {
        std::map<std::string, AppliedEffect> additions;
        try
        {
            for (const auto &[instanceId, desired] : desiredEffects)
            {
                if (appliedEffects_.contains(instanceId))
                {
                    continue;
                }
                ConditionCleanup cleanup = desired.definition->apply(resourceManager_, desired.context);
                if (!cleanup)
                {
                    throw std::invalid_argument("condition effect cleanup callback must not be empty: " + instanceId);
                }
                additions.emplace(instanceId, AppliedEffect{.cleanup = std::move(cleanup)});
            }
        }
        catch (...)
        {
            const std::exception_ptr error = std::current_exception();
            for (auto iterator = additions.rbegin(); iterator != additions.rend(); ++iterator)
            {
                try
                {
                    iterator->second.cleanup();
                }
                catch (...)
                {
                }
            }
            std::rethrow_exception(error);
        }

        for (auto iterator = appliedEffects_.begin(); iterator != appliedEffects_.end();)
        {
            if (desiredEffects.contains(iterator->first))
            {
                ++iterator;
                continue;
            }
            iterator->second.cleanup();
            iterator = appliedEffects_.erase(iterator);
        }
        for (auto &[instanceId, applied] : additions)
        {
            appliedEffects_.emplace(instanceId, std::move(applied));
        }
    }

    void ConditionManager::validateDerivedConditionGraph() const
    {
        enum class VisitState
        {
            Visiting,
            Visited
        };

        std::map<std::string, VisitState> states;
        std::function<void(const Condition &)> visit = [this, &states, &visit](const Condition &condition)
        {
            const auto state = states.find(condition.id_);
            if (state != states.end())
            {
                if (state->second == VisitState::Visiting)
                {
                    throw std::invalid_argument("circular derived condition reference: " + condition.id_);
                }
                return;
            }

            states.emplace(condition.id_, VisitState::Visiting);
            for (const ConditionStageDefinition &stage : condition.stages_)
            {
                for (const DerivedConditionDefinition &derivedCondition : stage.derivedConditions)
                {
                    const auto target = definitions_.find(derivedCondition.conditionId);
                    if (target != definitions_.end())
                    {
                        visit(target->second);
                    }
                }
            }
            states.at(condition.id_) = VisitState::Visited;
        };

        for (const auto &[conditionId, condition] : definitions_)
        {
            static_cast<void>(conditionId);
            visit(condition);
        }
    }

    const Condition &ConditionManager::definition(std::string_view conditionId) const
    {
        const std::string normalizedConditionId = normalize(conditionId);
        const auto iterator = definitions_.find(normalizedConditionId);
        if (iterator == definitions_.end())
        {
            throw std::invalid_argument("condition definition is not registered: " + normalizedConditionId);
        }
        return iterator->second;
    }

    const ConditionManager::StoredEntry &ConditionManager::storedEntry(std::string_view entryId) const
    {
        const std::string normalizedEntryId = normalize(entryId);
        const auto iterator = entries_.find(normalizedEntryId);
        if (iterator == entries_.end())
        {
            throw std::invalid_argument("condition entry is not registered: " + normalizedEntryId);
        }
        return iterator->second;
    }
}
