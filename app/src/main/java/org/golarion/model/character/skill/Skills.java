package org.golarion.model.character.skill;

import lombok.NonNull;

import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Skills
{
    private final EnumMap<SkillType, SkillEntry> genericEntries;
    private final EnumMap<SkillType, Map<String, SkillEntry>> specializedEntries;

    public Skills()
    {
        this.genericEntries = new EnumMap<>(SkillType.class);
        this.specializedEntries = new EnumMap<>(SkillType.class);

        for (SkillType skillType : SkillType.values())
        {
            genericEntries.put(skillType, new SkillEntry());

            if (skillType.isRequiresSpecialization())
            {
                specializedEntries.put(skillType, new LinkedHashMap<>());
            }
        }
    }

    public SkillEntry get(@NonNull SkillType skillType)
    {
        return genericEntries.get(skillType);
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

    public void addSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        specializedEntries.get(skillType).put(specialization.trim(), new SkillEntry());
    }

    public void removeSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        specializedEntries.get(skillType).remove(specialization.trim());
    }

    public int getTotalModifier(@NonNull SkillType skillType)
    {
        return get(skillType).getRanks()
                + (get(skillType).isClassSkill() && get(skillType).getRanks() > 0 ? 3 : 0)
                + get(skillType).getTotalBonus()
                - get(skillType).getTotalPenalty();
    }

    public int getTotalModifier(@NonNull SkillType skillType, @NonNull String specialization)
    {
        SkillEntry genericEntry = get(skillType);
        SkillEntry specializationEntry = getSpecialization(skillType, specialization);

        return specializationEntry.getRanks()
                + (genericEntry.isClassSkill() && specializationEntry.getRanks() > 0 ? 3 : 0)
                + specializationEntry.getTotalBonus()
                - specializationEntry.getTotalPenalty();
    }
}
