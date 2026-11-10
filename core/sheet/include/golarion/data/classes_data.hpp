#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/data/favored_classes_data.hpp"

#include <string>
#include <map>
#include <vector>

namespace golarion
{
    struct ClassChoiceData
    {
        std::string acquisitionId;
        std::string choiceId;
        std::vector<std::string> optionIds;
    };

    struct CharacterClassData
    {
        CharacterClassType type;
        int level;
        std::vector<ClassChoiceData> choices;
        std::vector<std::string> archetypeIds{};
        // One-based class level to raw die result; missing entries are not yet supplied.
        std::map<int, int> hitDieResults{};
    };

    struct ClassesData
    {
        std::vector<CharacterClassData> classes;
        FavoredClassesData favoredClasses{};
    };
}
