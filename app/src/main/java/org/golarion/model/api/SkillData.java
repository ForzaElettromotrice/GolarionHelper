package org.golarion.model.api;

import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.ability.AbilityType;

import java.util.List;

public record SkillData(
        SkillType skillType,
        String specialization,
        AbilityType abilityType,
        boolean classSkill,
        int ranks,
        int totalValue,
        List<ModifierData> modifiers
)
{
}
