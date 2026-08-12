#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct SkillAbilityOptionView
    {
        std::optional<std::string> replacementId;
        std::string source;
        AbilityType abilityType;
        int abilityModifier;
        int totalValue;
    };

    struct SkillClassSkillGrantView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
    };

    struct SkillView
    {
        SkillType type;
        std::optional<std::string> specializationId;
        std::string name;
        std::string resourceName;
        int ranks;
        bool classSkill;
        int classSkillBonus;
        bool trainedOnly;
        bool usable;
        bool custom;
        std::vector<SkillClassSkillGrantView> classSkillGrants;
        std::vector<SkillAbilityOptionView> abilityOptions;
        ModifierSetView modifiers;
    };
}
