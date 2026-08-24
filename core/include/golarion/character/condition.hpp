#pragma once

#include "golarion/data/condition_manager_save_data.hpp"
#include "golarion/view/condition_manager_view.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view ConditionEntriesResource = "condition.entries";

    class ResourceManager;

    enum class ConditionStackingMode
    {
        Shared,
        Escalating,
        PerEntry
    };

    enum class ConditionEntryOrigin
    {
        Manual,
        SourceControlled,
        Derived
    };

    std::string_view displayName(ConditionStackingMode mode);
    std::string_view displayName(ConditionEntryOrigin origin);

    struct ConditionEffectContext
    {
        std::string instanceId;
        std::string conditionId;
        std::string stageId;
        int stageSeverity;
        std::optional<std::string> entryId;
        std::optional<std::string> source;
        std::optional<std::string> parameter;
    };

    // Cleanup callbacks must not throw: once an external resource has been changed,
    // an arbitrary callback cannot be rolled back safely by ConditionManager.
    using ConditionCleanup = std::function<void()>;
    using ConditionApply = std::function<ConditionCleanup(ResourceManager &, const ConditionEffectContext &)>;

    struct ConditionEffectDefinition
    {
        std::string id;
        std::string description;
        ConditionApply apply;
    };

    struct DerivedConditionDefinition
    {
        std::string conditionId;
        int severity = 1;
    };

    struct ConditionStageDefinition
    {
        std::string id;
        std::string name;
        std::vector<DerivedConditionDefinition> derivedConditions{};
        std::vector<ConditionEffectDefinition> effects{};
    };

    struct ConditionDefinition
    {
        std::string id;
        std::string name;
        ConditionStackingMode stackingMode = ConditionStackingMode::Shared;
        std::vector<ConditionStageDefinition> stages;
        std::optional<std::string> entryParameterName = std::nullopt;
    };

    class Condition final
    {
    public:
        explicit Condition(ConditionDefinition definition);

    private:
        friend class ConditionManager;

        std::string id_;
        std::string name_;
        ConditionStackingMode stackingMode_;
        std::vector<ConditionStageDefinition> stages_;
        std::optional<std::string> entryParameterName_;
    };

    struct ConditionEntryDefinition
    {
        std::string id;
        std::string conditionId;
        std::string source;
        int severity = 1;
        bool contributesToEscalation = true;
        std::optional<std::string> stackingGroup = std::nullopt;
        std::optional<std::string> parameter = std::nullopt;
    };

    class ConditionEntry final
    {
    public:
        explicit ConditionEntry(ConditionEntryDefinition definition);

    private:
        friend class ConditionManager;

        std::string id_;
        std::string conditionId_;
        std::string source_;
        int severity_;
        bool contributesToEscalation_;
        std::optional<std::string> stackingGroup_;
        std::optional<std::string> parameter_;
    };

    class ConditionManager final
    {
    public:
        explicit ConditionManager(ResourceManager &resourceManager);
        ~ConditionManager();
        ConditionManager(const ConditionManager &) = delete;
        ConditionManager &operator=(const ConditionManager &) = delete;
        ConditionManager(ConditionManager &&) = delete;
        ConditionManager &operator=(ConditionManager &&) = delete;

        void registerDefinition(Condition condition);
        void addManualEntry(ConditionEntry entry);
        void removeManualEntry(std::string_view entryId);
        bool isActive(std::string_view conditionId) const;
        std::size_t entryCount(std::string_view conditionId) const;
        int effectiveSeverity(std::string_view conditionId) const;
        ConditionEntryOrigin entryOrigin(std::string_view entryId) const;
        ConditionManagerView toView() const;
        ConditionManagerSaveData toSaveData() const;
        void load(const ConditionManagerSaveData &data);

    private:
        struct StoredEntry
        {
            ConditionEntry entry;
            ConditionEntryOrigin origin;
        };

        struct DesiredEffect
        {
            const ConditionEffectDefinition *definition;
            ConditionEffectContext context;
        };

        struct AppliedEffect
        {
            ConditionCleanup cleanup;
        };

        void addEntry(ConditionEntry entry, ConditionEntryOrigin origin);
        void validateEntry(const ConditionEntry &entry) const;
        void addSourceControlledEntry(ConditionEntry entry);
        void removeSourceControlledEntry(std::string_view entryId);
        int effectiveSeverity(const Condition &condition, const std::map<std::string, StoredEntry> &entries) const;
        void reconcileDerivedEntries(std::map<std::string, StoredEntry> &entries) const;
        void commitEntries(std::map<std::string, StoredEntry> entries);
        std::map<std::string, DesiredEffect> desiredEffects(const std::map<std::string, StoredEntry> &entries) const;
        void transitionEffects(const std::map<std::string, DesiredEffect> &desiredEffects);
        void validateDerivedConditionGraph() const;
        const Condition &definition(std::string_view conditionId) const;
        const StoredEntry &storedEntry(std::string_view entryId) const;

        ResourceManager &resourceManager_;
        std::map<std::string, Condition> definitions_;
        std::map<std::string, StoredEntry> entries_;
        std::map<std::string, AppliedEffect> appliedEffects_;
    };
}
