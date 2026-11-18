#pragma once

#include "golarion/character/feat_definition.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct FeatRequirementView
    {
        std::string description;
        bool manual;
    };

    struct FeatCatalogOptionView
    {
        std::string id;
        std::string name;
        std::vector<FeatRequirementView> requirements;
        std::vector<std::string> notes;
    };

    struct FeatCatalogChoiceView
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<FeatCatalogOptionView> options;
    };

    struct FeatDefinitionView
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<FeatType> types;
        std::vector<FeatRequirementView> requirements;
        std::vector<FeatCatalogChoiceView> choices;
        FeatRepeatability repeatability = FeatRepeatability::Once;
        std::vector<std::string> notes;
    };
}
