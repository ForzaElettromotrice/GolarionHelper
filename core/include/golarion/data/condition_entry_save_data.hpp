#pragma once

#include <optional>
#include <string>

namespace golarion
{
    struct ConditionEntrySaveData
    {
        std::string id;
        std::string conditionId;
        std::string source;
        int severity;
        bool contributesToEscalation;
        std::optional<std::string> stackingGroup;
        std::optional<std::string> parameter;
    };
}
