#pragma once

#include <string>
#include <vector>

namespace golarion
{
    struct RacialChoiceOptionDefinition
    {
        std::string id;
        std::string name;
    };

    struct RacialChoiceDefinition
    {
        std::string id;
        std::string prompt;
        int selectionCount;
        std::vector<RacialChoiceOptionDefinition> options;
    };

    struct RacialElementDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<RacialChoiceDefinition> choices;
        std::vector<std::string> replaces;
    };

    struct RaceDefinition
    {
        std::string id;
        std::string name;
        std::vector<RacialElementDefinition> qualities;
        std::vector<RacialElementDefinition> standardFeatures;
        std::vector<RacialElementDefinition> alternateFeatures;
    };
}
