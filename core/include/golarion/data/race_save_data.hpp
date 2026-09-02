#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct RacialChoiceSelectionSaveData
    {
        std::string elementId;
        std::string choiceId;
        std::vector<std::string> optionIds;
    };

    struct RaceSaveData
    {
        std::optional<std::string> raceDefinitionId;
        std::vector<std::string> alternateFeatureIds;
        std::vector<RacialChoiceSelectionSaveData> choices;
    };
}
