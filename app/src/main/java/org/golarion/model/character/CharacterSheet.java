package org.golarion.model.character;

import lombok.Getter;
import org.golarion.model.character.ability.AbilityScores;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;

@Getter
public class CharacterSheet
{
    private String characterName;
    private final AbilityScores abilityScores;
    private final Skills skills;

    public CharacterSheet(String characterName)
    {
        this.abilityScores = new AbilityScores();
        this.skills = new Skills();
        setCharacterName(characterName);
    }

    public void setCharacterName(String characterName)
    {
        String normalizedName = normalize(characterName);
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("characterName must not be blank");
        }

        this.characterName = normalizedName;
    }

    public int getSkillModifier(SkillType skillType)
    {
        return skills.getTotalModifier(skillType, abilityScores);
    }

    public int getSkillModifier(SkillType skillType, String specialization)
    {
        return skills.getTotalModifier(skillType, specialization, abilityScores);
    }

    private String normalize(String value)
    {
        return value == null ? "" : value.trim();
    }
}
