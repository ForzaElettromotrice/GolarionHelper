#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct RaceSummaryView
    {
        std::string id;
        std::string name;
    };

    struct RacialChoiceOptionView
    {
        std::string id;
        std::string name;
    };

    struct RacialChoiceSelectionView
    {
        std::string id;
        std::string prompt;
        std::vector<RacialChoiceOptionView> selectedOptions;
    };

    struct MissingRacialChoiceView
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<RacialChoiceOptionView> options;
    };

    struct RacialElementView
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<RacialChoiceSelectionView> choices;
        std::vector<MissingRacialChoiceView> missingChoices;
    };

    struct RaceView
    {
        std::optional<RaceSummaryView> race;
        std::vector<RacialElementView> activeElements;
    };
}
