#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    enum class ConditionStackingMode;
    enum class ConditionEntryOrigin;

    struct ConditionEntryView
    {
        std::string id;
        std::string source;
        int severity;
        bool contributesToEscalation;
        std::optional<std::string> stackingGroup;
        ConditionEntryOrigin origin;
        std::optional<std::string> parameter;
    };

    struct DerivedConditionView
    {
        std::string conditionId;
        int severity;
    };

    struct ConditionEffectInstanceView
    {
        std::string instanceId;
        std::optional<std::string> entryId;
        std::optional<std::string> source;
    };

    struct ConditionEffectView
    {
        std::string id;
        std::string description;
        std::vector<ConditionEffectInstanceView> activeInstances;
    };

    struct ConditionStageView
    {
        std::string id;
        std::string name;
        int severity;
        bool active;
        std::vector<DerivedConditionView> derivedConditions;
        std::vector<ConditionEffectView> effects;
    };

    struct ConditionView
    {
        std::string id;
        std::string name;
        ConditionStackingMode stackingMode;
        int effectiveSeverity;
        std::vector<ConditionStageView> stages;
        std::vector<ConditionEntryView> entries;
        std::optional<std::string> entryParameterName;
    };
}
