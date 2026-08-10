#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>

namespace golarion
{
    struct SkillView
    {
        SkillType type;
        std::optional<std::string> specializationId;
        std::string name;
        std::string resourceName;
        AbilityType abilityType;
        int abilityModifier;
        int ranks;
        bool classSkill;
        int classSkillBonus;
        int totalValue;
        bool trainedOnly;
        bool usable;
        bool custom;
        ModifierSetView modifiers;
    };
}
