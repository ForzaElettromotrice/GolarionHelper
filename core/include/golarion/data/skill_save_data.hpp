#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/skill.hpp"

#include <optional>
#include <string>

namespace golarion
{
    struct SkillSaveData
    {
        SkillType type;
        std::optional<std::string> specializationId;
        std::optional<std::string> specialization;
        AbilityType abilityType;
        int ranks;
        bool classSkill;
        bool custom;
    };
}
