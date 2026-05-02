package org.golarion.model.character;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.AbilityData;
import org.golarion.model.api.SkillData;
import org.golarion.model.character.ability.Ability;
import org.golarion.model.character.ability.AbilityScore;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Penalty;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;

import java.util.EnumMap;
import java.util.List;
import java.util.UUID;

public class CharacterSheet
{
    private final EnumMap<Ability, AbilityScore> abilityScores;
    private final Skills skills;
    @Getter
    private String characterName;

    public CharacterSheet()
    {
        this.characterName = "Franco";
        this.abilityScores = new EnumMap<>(Ability.class);
        for (Ability ability : Ability.values())
        {
            abilityScores.put(ability, new AbilityScore(10));
        }
        this.skills = new Skills();
        setCharacterName(characterName);
    }

    public void setCharacterName(@NonNull String characterName)
    {
        String normalizedName = characterName.trim();
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("characterName must not be blank");
        }

        this.characterName = normalizedName;
    }

    public UUID addAbilityBonus(@NonNull Ability ability, @NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        getAbilityScore(ability).addBonus(bonus);
        return bonus.getId();
    }

    public void removeAbilityBonus(@NonNull Ability ability, @NonNull UUID bonusId)
    {
        getAbilityScore(ability).removeBonus(bonusId);
    }

    public void setAbiltyBonusEnable(@NonNull Ability ability, @NonNull UUID bonusId, boolean enabled)
    {
        getAbilityScore(ability).setBonusEnabled(bonusId, enabled);
    }

    public UUID addAbilityPenalty(@NonNull Ability ability, @NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        getAbilityScore(ability).addPenalty(penalty);
        return penalty.getId();
    }

    public void removeAbilityPenalty(@NonNull Ability ability, @NonNull UUID penaltyId)
    {
        getAbilityScore(ability).removePenalty(penaltyId);
    }

    public void setAbiltyPenaltyEnable(@NonNull Ability ability, @NonNull UUID penaltyId, boolean enabled)
    {
        getAbilityScore(ability).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setAbilityBaseValue(@NonNull Ability ability, int baseValue)
    {
        getAbilityScore(ability).setBaseValue(baseValue);
    }

    public AbilityData getAbility(Ability ability)
    {
        return getAbilityScore(ability).toData(ability);
    }

    public List<String> getSkillSpecializations(@NonNull SkillType skillType)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        return skills.getSpecializations(skillType);
    }

    public void addSkillSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        if (specialization.trim().isBlank())
        {
            throw new IllegalArgumentException("specialization must not be blank");
        }
        if (skills.getSpecializations(skillType).contains(specialization.trim()))
        {
            throw new IllegalArgumentException("specialization already exists");
        }
        skills.addSpecialization(skillType, specialization.trim());
        skills.getSpecialization(skillType, specialization.trim()).setClassSkill(skills.get(skillType).isClassSkill());
    }

    public void removeSkillSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        skills.removeSpecialization(skillType, specialization);
    }

    public UUID addSkillBonus(@NonNull SkillType skillType, @NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        skills.get(skillType).addBonus(bonus);
        return bonus.getId();
    }

    public UUID addSkillBonus(@NonNull SkillType skillType, @NonNull String specialization, @NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        skills.getSpecialization(skillType, specialization).addBonus(bonus);
        return bonus.getId();
    }

    public void removeSkillBonus(@NonNull SkillType skillType, @NonNull UUID bonusId)
    {
        skills.get(skillType).removeBonus(bonusId);
    }

    public void removeSkillBonus(@NonNull SkillType skillType, @NonNull String specialization, @NonNull UUID bonusId)
    {
        skills.getSpecialization(skillType, specialization).removeBonus(bonusId);
    }

    public void setSkillBonusEnable(@NonNull SkillType skillType, @NonNull UUID bonusId, boolean enabled)
    {
        skills.get(skillType).setBonusEnabled(bonusId, enabled);
    }

    public void setSkillBonusEnable(@NonNull SkillType skillType, @NonNull String specialization, @NonNull UUID bonusId, boolean enabled)
    {
        skills.getSpecialization(skillType, specialization).setBonusEnabled(bonusId, enabled);
    }

    public UUID addSkillPenalty(@NonNull SkillType skillType, @NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        skills.get(skillType).addPenalty(penalty);
        return penalty.getId();
    }

    public UUID addSkillPenalty(@NonNull SkillType skillType, @NonNull String specialization, @NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        skills.getSpecialization(skillType, specialization).addPenalty(penalty);
        return penalty.getId();
    }

    public void removeSkillPenalty(@NonNull SkillType skillType, @NonNull UUID penaltyId)
    {
        skills.get(skillType).removePenalty(penaltyId);
    }

    public void removeSkillPenalty(@NonNull SkillType skillType, @NonNull String specialization, @NonNull UUID penaltyId)
    {
        skills.getSpecialization(skillType, specialization).removePenalty(penaltyId);
    }

    public void setSkillPenaltyEnable(@NonNull SkillType skillType, @NonNull UUID penaltyId, boolean enabled)
    {
        skills.get(skillType).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setSkillPenaltyEnable(@NonNull SkillType skillType, @NonNull String specialization, @NonNull UUID penaltyId, boolean enabled)
    {
        skills.getSpecialization(skillType, specialization).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setSkillRanks(@NonNull SkillType skillType, int ranks)
    {
        if (skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + "cannot have ranks without specialization");
        }
        skills.get(skillType).setRanks(ranks);
    }

    public void setSkillRanks(@NonNull SkillType skillType, @NonNull String specialization, int ranks)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        skills.getSpecialization(skillType, specialization).setRanks(ranks);
    }

    public void setSkillClassSkill(@NonNull SkillType skillType, boolean classSkill)
    {
        skills.get(skillType).setClassSkill(classSkill);
        if (skillType.isRequiresSpecialization())
        {
            for (String specialization : skills.getSpecializations(skillType))
            {
                skills.getSpecialization(skillType, specialization).setClassSkill(classSkill);
            }
        }
    }

    public SkillData getSkill(@NonNull SkillType skillType)
    {
        int totalModifier = getAbilityScore(skillType.getKeyAbility()).getModifier()
                + skills.getTotalModifier(skillType);

        return skills.get(skillType).toData(skillType, "", totalModifier);
    }

    public SkillData getSkill(@NonNull SkillType skillType, @NonNull String specialization)
    {
        int totalModifier = getAbilityScore(skillType.getKeyAbility()).getModifier()
                + skills.getTotalModifier(skillType, specialization);

        return skills.getSpecialization(skillType, specialization).toData(skillType, specialization, totalModifier);
    }

    private AbilityScore getAbilityScore(@NonNull Ability ability)
    {
        return abilityScores.get(ability);
    }
}
