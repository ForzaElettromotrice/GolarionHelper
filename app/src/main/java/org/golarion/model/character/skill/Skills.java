package org.golarion.model.character.skill;

import lombok.NonNull;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Skills
{
    private class SkillGroupModifierTarget implements ModifierTarget
    {
        private final List<SkillType> skillTypes;

        private SkillGroupModifierTarget(@NonNull List<SkillType> skillTypes)
        {
            this.skillTypes = List.copyOf(skillTypes);
        }

        @Override
        public void addModifier(@NonNull Modifier modifier)
        {
            for (SkillType skillType : skillTypes)
            {
                get(skillType).addModifier(modifier);
            }
        }

        @Override
        public void removeModifier(@NonNull java.util.UUID modifierId)
        {
            for (SkillType skillType : skillTypes)
            {
                get(skillType).removeModifier(modifierId);
            }
        }
    }

    private final EnumMap<SkillType, SkillEntry> genericEntries;
    private final EnumMap<SkillType, Map<String, SkillEntry>> specializedEntries;
    private final SkillGroupModifierTarget allSkillsTarget;
    private final SkillGroupModifierTarget knowledgeSkillsTarget;
    private final SkillGroupModifierTarget armorCheckPenaltySkillsTarget;

    public Skills()
    {
        this.genericEntries = new EnumMap<>(SkillType.class);
        this.specializedEntries = new EnumMap<>(SkillType.class);

        for (SkillType skillType : SkillType.values())
        {
            genericEntries.put(skillType, new SkillEntry(skillType.getKeyAbility()));

            if (skillType.isRequiresSpecialization())
            {
                specializedEntries.put(skillType, new LinkedHashMap<>());
            }
        }

        this.allSkillsTarget = new SkillGroupModifierTarget(List.of(SkillType.values()));
        this.knowledgeSkillsTarget = new SkillGroupModifierTarget(
                java.util.Arrays.stream(SkillType.values())
                        .filter(SkillType::isKnowledge)
                        .toList()
        );
        this.armorCheckPenaltySkillsTarget = new SkillGroupModifierTarget(
                java.util.Arrays.stream(SkillType.values())
                        .filter(SkillType::isArmorCheckPenaltyApplied)
                        .toList()
        );
    }

    public SkillEntry get(@NonNull SkillType skillType)
    {
        return genericEntries.get(skillType);
    }

    public void registerModifierTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget("Skills", allSkillsTarget);
        targetManager.registerModifierTarget("Knowledge", knowledgeSkillsTarget);
        targetManager.registerModifierTarget("ArmorCheckPenalty", armorCheckPenaltySkillsTarget);
        for (SkillType skillType : SkillType.values())
        {
            targetManager.registerModifierTarget(skillType.toString(), get(skillType));
        }
    }

    public SkillEntry getSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }

        SkillEntry specializationEntry = specializedEntries.get(skillType).get(specialization.trim());
        if (specializationEntry == null)
        {
            throw new IllegalArgumentException("specialization not found for " + skillType + ": " + specialization);
        }

        return specializationEntry;
    }

    public List<String> getSpecializations(@NonNull SkillType skillType)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }

        return specializedEntries.get(skillType).keySet().stream().toList();
    }

    public void setClassSkill(@NonNull SkillType skillType, boolean classSkill)
    {
        get(skillType).setClassSkill(classSkill);
        if (skillType.isRequiresSpecialization())
        {
            for (String specialization : getSpecializations(skillType))
            {
                getSpecialization(skillType, specialization).setClassSkill(classSkill);
            }
        }
    }

    public void setAbilityType(@NonNull SkillType skillType, @NonNull org.golarion.model.character.ability.AbilityType abilityType)
    {
        get(skillType).setAbilityType(abilityType);
        if (skillType.isRequiresSpecialization())
        {
            for (String specialization : getSpecializations(skillType))
            {
                getSpecialization(skillType, specialization).setAbilityType(abilityType);
            }
        }
    }

    public void addSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        if (specialization.trim().isBlank())
        {
            throw new IllegalArgumentException("specialization must not be blank");
        }
        if (getSpecializations(skillType).contains(specialization.trim()))
        {
            throw new IllegalArgumentException("specialization already exists");
        }
        specializedEntries.get(skillType).put(specialization.trim(), new SkillEntry(skillType.getKeyAbility()));
        getSpecialization(skillType, specialization.trim()).setClassSkill(get(skillType).isClassSkill());
        getSpecialization(skillType, specialization.trim()).setAbilityType(get(skillType).getAbilityType());
    }

    public void removeSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        specializedEntries.get(skillType).remove(specialization.trim());
    }
}
