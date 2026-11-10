#pragma once

#include "golarion/character/character_class_type.hpp"
#include <set>
#include <string>
#include <vector>

namespace golarion
{
    struct FavoredClassBonusSelectionData
    {
        CharacterClassType type;
        int classLevel;
        std::string optionId;
    };

    struct FavoredClassesData
    {
        std::set<CharacterClassType> selectedClasses{};
        std::vector<FavoredClassBonusSelectionData> bonuses{};
    };
}
