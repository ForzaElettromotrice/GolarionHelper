package org.golarion.model.api;

import org.golarion.model.character.skill.SkillType;

import java.util.List;

public record SkillData(
        SkillType skillType,
        String specialization,
        boolean classSkill,
        int ranks,
        int totalModifier,
        List<BonusData> bonuses,
        List<PenaltyData> penalties
)
{
}
