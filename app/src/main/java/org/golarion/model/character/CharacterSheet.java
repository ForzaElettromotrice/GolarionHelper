package org.golarion.model.character;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.*;
import org.golarion.model.character.ability.AbilityScore;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.armorclass.ArmorClassEntry;
import org.golarion.model.character.hitpoints.HitPointField;
import org.golarion.model.character.hitpoints.HitPointsEntry;
import org.golarion.model.character.initiative.InitiativeEntry;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Penalty;
import org.golarion.model.character.savingthrow.SavingThrowEntry;
import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;

import java.util.EnumMap;
import java.util.List;
import java.util.UUID;

public class CharacterSheet
{
    private final EnumMap<AbilityType, AbilityScore> abilityScores;
    private final EnumMap<SavingThrowType, SavingThrowEntry> savingThrows;
    private final ArmorClassEntry armorClass;
    private final HitPointsEntry hitPoints;
    private final InitiativeEntry initiative;
    private final Skills skills;
    @Getter
    private String characterName;

    public CharacterSheet()
    {
        this.characterName = "Franco";
        this.abilityScores = new EnumMap<>(AbilityType.class);
        for (AbilityType abilityType : AbilityType.values())
        {
            abilityScores.put(abilityType, new AbilityScore(10));
        }
        this.savingThrows = new EnumMap<>(SavingThrowType.class);
        for (SavingThrowType savingThrowType : SavingThrowType.values())
        {
            savingThrows.put(savingThrowType, new SavingThrowEntry());
        }
        this.armorClass = new ArmorClassEntry();
        this.hitPoints = new HitPointsEntry();
        this.initiative = new InitiativeEntry();
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

    public UUID addAbilityBonus(@NonNull AbilityType abilityType, @NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        getAbilityScore(abilityType).addBonus(bonus);
        return bonus.getId();
    }

    public void removeAbilityBonus(@NonNull AbilityType abilityType, @NonNull UUID bonusId)
    {
        getAbilityScore(abilityType).removeBonus(bonusId);
    }

    public void setAbiltyBonusEnable(@NonNull AbilityType abilityType, @NonNull UUID bonusId, boolean enabled)
    {
        getAbilityScore(abilityType).setBonusEnabled(bonusId, enabled);
    }

    public UUID addAbilityPenalty(@NonNull AbilityType abilityType, @NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        getAbilityScore(abilityType).addPenalty(penalty);
        return penalty.getId();
    }

    public void removeAbilityPenalty(@NonNull AbilityType abilityType, @NonNull UUID penaltyId)
    {
        getAbilityScore(abilityType).removePenalty(penaltyId);
    }

    public void setAbiltyPenaltyEnable(@NonNull AbilityType abilityType, @NonNull UUID penaltyId, boolean enabled)
    {
        getAbilityScore(abilityType).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setAbilityBaseValue(@NonNull AbilityType abilityType, int baseValue)
    {
        getAbilityScore(abilityType).setBaseValue(baseValue);
    }

    public AbilityData getAbility(AbilityType abilityType)
    {
        return getAbilityScore(abilityType).toData(abilityType);
    }

    public void setSavingThrowBaseValue(@NonNull SavingThrowType savingThrowType, int baseValue)
    {
        getSavingThrowEntry(savingThrowType).setBaseValue(baseValue);
    }

    public UUID addSavingThrowBonus(@NonNull SavingThrowType savingThrowType, @NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        getSavingThrowEntry(savingThrowType).addBonus(bonus);
        return bonus.getId();
    }

    public void removeSavingThrowBonus(@NonNull SavingThrowType savingThrowType, @NonNull UUID bonusId)
    {
        getSavingThrowEntry(savingThrowType).removeBonus(bonusId);
    }

    public void setSavingThrowBonusEnable(@NonNull SavingThrowType savingThrowType, @NonNull UUID bonusId, boolean enabled)
    {
        getSavingThrowEntry(savingThrowType).setBonusEnabled(bonusId, enabled);
    }

    public UUID addSavingThrowPenalty(@NonNull SavingThrowType savingThrowType, @NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        getSavingThrowEntry(savingThrowType).addPenalty(penalty);
        return penalty.getId();
    }

    public void removeSavingThrowPenalty(@NonNull SavingThrowType savingThrowType, @NonNull UUID penaltyId)
    {
        getSavingThrowEntry(savingThrowType).removePenalty(penaltyId);
    }

    public void setSavingThrowPenaltyEnable(@NonNull SavingThrowType savingThrowType, @NonNull UUID penaltyId, boolean enabled)
    {
        getSavingThrowEntry(savingThrowType).setPenaltyEnabled(penaltyId, enabled);
    }

    public SavingThrowData getSavingThrow(@NonNull SavingThrowType savingThrowType)
    {
        int totalModifier = getAbilityScore(savingThrowType.getKeyAbility()).getModifier()
                + getSavingThrowEntry(savingThrowType).getTotalValue();

        return getSavingThrowEntry(savingThrowType).toData(savingThrowType, totalModifier);
    }

    public UUID addArmorClassBonus(@NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        armorClass.addBonus(bonus);
        return bonus.getId();
    }

    public void removeArmorClassBonus(@NonNull UUID bonusId)
    {
        armorClass.removeBonus(bonusId);
    }

    public void setArmorClassBonusEnable(@NonNull UUID bonusId, boolean enabled)
    {
        armorClass.setBonusEnabled(bonusId, enabled);
    }

    public UUID addArmorClassPenalty(@NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        armorClass.addPenalty(penalty);
        return penalty.getId();
    }

    public void removeArmorClassPenalty(@NonNull UUID penaltyId)
    {
        armorClass.removePenalty(penaltyId);
    }

    public void setArmorClassPenaltyEnable(@NonNull UUID penaltyId, boolean enabled)
    {
        armorClass.setPenaltyEnabled(penaltyId, enabled);
    }

    public ArmorClassData getArmorClass()
    {
        int totalValue = getAbilityScore(AbilityType.DEXTERITY).getModifier()
                + armorClass.getACTotalValue();
        int touchValue = getAbilityScore(AbilityType.DEXTERITY).getModifier()
                + armorClass.getACTouch();
        int flatFootedValue = armorClass.getACFlatFooted();

        return armorClass.toData(totalValue, touchValue, flatFootedValue);
    }

    public void setHitPoints(@NonNull HitPointField field, int value)
    {
        hitPoints.set(field, value);
    }

    public void changeHitPoints(@NonNull HitPointField field, int delta)
    {
        hitPoints.change(field, delta);
    }

    public HitPointsData getHitPoints()
    {
        return hitPoints.toData(hitPoints.getMaxHp());
    }

    public void setInitiativeBaseValue(int baseValue)
    {
        initiative.setBaseValue(baseValue);
    }

    public UUID addInitiativeBonus(@NonNull String source, @NonNull BonusType bonusType, int value, @NonNull String description)
    {
        Bonus bonus = new Bonus(source, bonusType, value, true, description);
        initiative.addBonus(bonus);
        return bonus.getId();
    }

    public void removeInitiativeBonus(@NonNull UUID bonusId)
    {
        initiative.removeBonus(bonusId);
    }

    public void setInitiativeBonusEnable(@NonNull UUID bonusId, boolean enabled)
    {
        initiative.setBonusEnabled(bonusId, enabled);
    }

    public UUID addInitiativePenalty(@NonNull String source, int value, @NonNull String description)
    {
        Penalty penalty = new Penalty(source, value, true, description);
        initiative.addPenalty(penalty);
        return penalty.getId();
    }

    public void removeInitiativePenalty(@NonNull UUID penaltyId)
    {
        initiative.removePenalty(penaltyId);
    }

    public void setInitiativePenaltyEnable(@NonNull UUID penaltyId, boolean enabled)
    {
        initiative.setPenaltyEnabled(penaltyId, enabled);
    }

    public InitiativeData getInitiative()
    {
        int totalModifier = getAbilityScore(AbilityType.DEXTERITY).getModifier()
                + initiative.getTotalValue();

        return initiative.toData(totalModifier);
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

    private AbilityScore getAbilityScore(@NonNull AbilityType abilityType)
    {
        return abilityScores.get(abilityType);
    }

    private SavingThrowEntry getSavingThrowEntry(@NonNull SavingThrowType savingThrowType)
    {
        return savingThrows.get(savingThrowType);
    }
}
