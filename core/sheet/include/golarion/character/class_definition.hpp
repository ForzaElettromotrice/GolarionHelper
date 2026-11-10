#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/class_feature_definition.hpp"
#include "golarion/character/saving_throw.hpp"
#include "golarion/character/skill.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace golarion
{
    enum class ClassHitDie
    {
        D6 = 6,
        D8 = 8,
        D10 = 10,
        D12 = 12
    };

    struct ClassLevelDefinition
    {
        int level;
        // Total contributions at this class level, not per-level increments.
        int baseAttackBonus;
        std::map<SavingThrowType, int> baseSavingThrows;
    };

    struct ClassDefinition
    {
        CharacterClassType type;
        std::string name;
        std::string description;
        ClassHitDie hitDie;
        // Base amount, before Intelligence and other source contributions.
        int skillRanksPerLevel;
        std::set<SkillType> classSkills;
        std::vector<ClassLevelDefinition> progression;
        std::vector<ClassFeatureDefinition> features{};
    };
}
