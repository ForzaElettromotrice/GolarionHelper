#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/view/favored_classes_view.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace golarion
{
    struct ClassOptionView
    {
        std::string id;
        std::string name;
        std::string description;
        bool selected;
    };

    struct ClassChoiceView
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::size_t remainingSelections;
        std::vector<ClassOptionView> options;
    };

    struct ClassAcquisitionView
    {
        std::string id;
        int level;
        bool active;
        std::string parentAcquisitionId;
        std::string parentChoiceId;
        std::string parentOptionId;
        std::vector<ClassChoiceView> choices;
    };

    struct ClassFeatureView
    {
        std::string id;
        std::string name;
        std::string description;
        std::vector<ClassAcquisitionView> acquisitions;
    };

    struct ArchetypeView
    {
        std::string id;
        std::string name;
        std::string description;
    };

    struct CharacterClassView
    {
        CharacterClassType type;
        std::string name;
        int level;
        std::vector<ClassFeatureView> features;
        std::vector<ArchetypeView> archetypes{};
        std::map<int, int> hitDieResults{};
    };

    struct ClassesView
    {
        int level;
        std::vector<CharacterClassView> classes;
        FavoredClassesView favoredClasses{};
    };
}
