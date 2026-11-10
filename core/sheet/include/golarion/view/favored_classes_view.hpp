#pragma once

#include "golarion/data/favored_classes_data.hpp"

namespace golarion
{
    struct FavoredClassBonusOptionView
    {
        std::string id;
        std::string name;
        std::string description;
        bool available;
        int selectionsPerBenefit;
        int selections;
        int benefits;
    };

    struct FavoredClassBonusSelectionView
    {
        int classLevel;
        std::string optionId;
        bool active;
    };

    struct FavoredClassView
    {
        CharacterClassType type;
        bool favored;
        int eligibleLevels;
        int unassignedLevels;
        std::vector<FavoredClassBonusOptionView> options;
        std::vector<FavoredClassBonusSelectionView> selections;
    };

    struct FavoredClassesView
    {
        std::set<CharacterClassType> selectedClasses;
        int choiceCapacity;
        bool tooManySelectedClasses;
        std::vector<FavoredClassView> classes;
    };
}
