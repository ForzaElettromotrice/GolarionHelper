package org.golarion.model.character.skill;

import lombok.Getter;
import org.golarion.model.character.ability.AbilityScores;
import org.golarion.model.character.modifier.BonusType;

import java.util.*;

@Getter
public class Skills
{
    private final EnumMap<SkillType, Boolean> classSkillFlags;
    private final EnumMap<SkillType, SkillEntry> genericEntries;
    private final EnumMap<SkillType, SkillAdjustmentSet> familyAdjustments;
    private final EnumMap<SkillType, LinkedHashMap<String, SkillEntry>> specializedEntries;
    private int armorCheckPenalty;

    public Skills()
    {
        this.classSkillFlags = new EnumMap<>(SkillType.class);
        this.genericEntries = new EnumMap<>(SkillType.class);
        this.familyAdjustments = new EnumMap<>(SkillType.class);
        this.specializedEntries = new EnumMap<>(SkillType.class);
        this.armorCheckPenalty = 0;

        for (SkillType skillType : SkillType.values())
        {
            classSkillFlags.put(skillType, false);
            familyAdjustments.put(skillType, new SkillAdjustmentSet());

            if (skillType.isRequiresSpecialization())
            {
                specializedEntries.put(skillType, new LinkedHashMap<>());
                continue;
            }

            genericEntries.put(skillType, new SkillEntry());
        }
    }

    public SkillEntry get(SkillType skillType)
    {
        validateSkillType(skillType);

        if (skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " requires a specialization");
        }

        return genericEntries.get(skillType);
    }

    public SkillEntry getSpecialization(SkillType skillType, String specialization)
    {
        validateSpecialization(skillType, specialization);
        return getSpecializedEntry(skillType, normalize(specialization));
    }

    public SkillEntry getOrCreateSpecialization(SkillType skillType, String specialization)
    {
        validateSpecialization(skillType, specialization);
        return getOrCreateSpecializedEntry(skillType, normalize(specialization));
    }

    public void removeSpecialization(SkillType skillType, String specialization)
    {
        validateSpecialization(skillType, specialization);
        specializedEntries.get(skillType).remove(normalize(specialization));
    }

    public Map<String, SkillEntry> getSpecializations(SkillType skillType)
    {
        validateSkillType(skillType);

        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }

        return Collections.unmodifiableMap(specializedEntries.get(skillType));
    }

    public SkillAdjustmentSet getFamilyAdjustments(SkillType skillType)
    {
        validateSkillType(skillType);
        return familyAdjustments.get(skillType);
    }

    public void setRanks(SkillType skillType, int ranks)
    {
        get(skillType).setRanks(ranks);
    }

    public void setRanks(SkillType skillType, String specialization, int ranks)
    {
        getOrCreateSpecialization(skillType, specialization).setRanks(ranks);
    }

    public void setClassSkill(SkillType skillType, boolean classSkill)
    {
        validateSkillType(skillType);
        classSkillFlags.put(skillType, classSkill);

        if (skillType.isRequiresSpecialization())
        {
            specializedEntries.get(skillType).values()
                    .forEach(entry -> entry.setClassSkill(classSkill));
            return;
        }

        get(skillType).setClassSkill(classSkill);
    }

    public void setArmorCheckPenalty(int armorCheckPenalty)
    {
        if (armorCheckPenalty < 0)
        {
            throw new IllegalArgumentException("armorCheckPenalty must not be negative");
        }

        this.armorCheckPenalty = armorCheckPenalty;
    }

    public void addBonus(SkillType skillType, SkillBonus bonus)
    {
        get(skillType).addBonus(bonus);
    }

    public void addBonus(SkillType skillType, String specialization, SkillBonus bonus)
    {
        getOrCreateSpecialization(skillType, specialization).addBonus(bonus);
    }

    public void addFamilyBonus(SkillType skillType, SkillBonus bonus)
    {
        getFamilyAdjustments(skillType).addBonus(bonus);
    }

    public void removeBonus(SkillType skillType, UUID bonusId)
    {
        get(skillType).removeBonus(bonusId);
    }

    public void removeBonus(SkillType skillType, String specialization, UUID bonusId)
    {
        getSpecialization(skillType, specialization).removeBonus(bonusId);
    }

    public void removeFamilyBonus(SkillType skillType, UUID bonusId)
    {
        getFamilyAdjustments(skillType).removeBonus(bonusId);
    }

    public void setBonusEnabled(SkillType skillType, UUID bonusId, boolean enabled)
    {
        get(skillType).setBonusEnabled(bonusId, enabled);
    }

    public void setBonusEnabled(SkillType skillType, String specialization, UUID bonusId, boolean enabled)
    {
        getSpecialization(skillType, specialization).setBonusEnabled(bonusId, enabled);
    }

    public void setFamilyBonusEnabled(SkillType skillType, UUID bonusId, boolean enabled)
    {
        getFamilyAdjustments(skillType).setBonusEnabled(bonusId, enabled);
    }

    public void addPenalty(SkillType skillType, SkillPenalty penalty)
    {
        get(skillType).addPenalty(penalty);
    }

    public void addPenalty(SkillType skillType, String specialization, SkillPenalty penalty)
    {
        getOrCreateSpecialization(skillType, specialization).addPenalty(penalty);
    }

    public void addFamilyPenalty(SkillType skillType, SkillPenalty penalty)
    {
        getFamilyAdjustments(skillType).addPenalty(penalty);
    }

    public void removePenalty(SkillType skillType, UUID penaltyId)
    {
        get(skillType).removePenalty(penaltyId);
    }

    public void removePenalty(SkillType skillType, String specialization, UUID penaltyId)
    {
        getSpecialization(skillType, specialization).removePenalty(penaltyId);
    }

    public void removeFamilyPenalty(SkillType skillType, UUID penaltyId)
    {
        getFamilyAdjustments(skillType).removePenalty(penaltyId);
    }

    public void setPenaltyEnabled(SkillType skillType, UUID penaltyId, boolean enabled)
    {
        get(skillType).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setPenaltyEnabled(SkillType skillType, String specialization, UUID penaltyId, boolean enabled)
    {
        getSpecialization(skillType, specialization).setPenaltyEnabled(penaltyId, enabled);
    }

    public void setFamilyPenaltyEnabled(SkillType skillType, UUID penaltyId, boolean enabled)
    {
        getFamilyAdjustments(skillType).setPenaltyEnabled(penaltyId, enabled);
    }

    private SkillEntry getSpecializedEntry(SkillType skillType, String specialization)
    {
        SkillEntry entry = specializedEntries.get(skillType).get(specialization);
        if (entry == null)
        {
            throw new IllegalArgumentException("specialization not found for " + skillType + ": " + specialization);
        }

        return entry;
    }

    private SkillEntry getOrCreateSpecializedEntry(SkillType skillType, String specialization)
    {
        return specializedEntries.get(skillType)
                .computeIfAbsent(specialization, ignored -> new SkillEntry(0, isClassSkill(skillType)));
    }

    public boolean isClassSkill(SkillType skillType)
    {
        validateSkillType(skillType);
        return classSkillFlags.get(skillType);
    }


    public int getTotalModifier(SkillType skillType, AbilityScores abilityScores)
    {
        validateSkillType(skillType);
        return skillType.isRequiresSpecialization()
            ? calculateFamilyModifier(skillType, abilityScores)
            : calculateTotalModifier(skillType, get(skillType), abilityScores);
    }

    public int getTotalModifier(SkillType skillType, String specialization, AbilityScores abilityScores)
    {
        validateSpecialization(skillType, specialization);
        return calculateTotalModifier(skillType, getSpecialization(skillType, specialization), abilityScores);
    }

    private void validateSkillType(SkillType skillType)
    {
        if (skillType == null)
        {
            throw new IllegalArgumentException("skillType must not be null");
        }
    }

    private int calculateTotalModifier(SkillType skillType, SkillEntry entry, AbilityScores abilityScores)
    {
        return calculateBaseModifier(skillType, abilityScores)
            + entry.getRanks()
            + calculateClassSkillBonus(entry)
            + calculateTotalBonus(entry, getFamilyAdjustments(skillType))
            - calculateTotalPenalty(entry, getFamilyAdjustments(skillType));
    }

    private int calculateFamilyModifier(SkillType skillType, AbilityScores abilityScores)
    {
        return calculateBaseModifier(skillType, abilityScores)
            + calculateTotalBonus(getFamilyAdjustments(skillType).getBonuses())
            - calculatePenaltyValue(getFamilyAdjustments(skillType).getPenalties());
    }

    private int calculateBaseModifier(SkillType skillType, AbilityScores abilityScores)
    {
        if (abilityScores == null)
        {
            throw new IllegalArgumentException("abilityScores must not be null");
        }

        return abilityScores.get(skillType.getKeyAbility()).getModifier()
            - calculateArmorCheckPenalty(skillType);
    }

    private int calculateClassSkillBonus(SkillEntry entry)
    {
        return entry.isClassSkill() && entry.getRanks() > 0 ? 3 : 0;
    }

    private int calculateTotalBonus(List<SkillBonus> bonuses)
    {
        Map<BonusType, List<SkillBonus>> bonusesByType = new EnumMap<>(BonusType.class);

        collectBonuses(bonuses, bonusesByType);

        return bonusesByType.entrySet().stream()
            .mapToInt(entrySet -> calculateBonusGroup(entrySet.getKey(), entrySet.getValue()))
            .sum();
    }

    private int calculateTotalBonus(SkillEntry entry, SkillAdjustmentSet adjustmentSet)
    {
        Map<BonusType, List<SkillBonus>> bonusesByType = new EnumMap<>(BonusType.class);

        collectBonuses(entry.getBonuses(), bonusesByType);
        collectBonuses(adjustmentSet.getBonuses(), bonusesByType);

        return bonusesByType.entrySet().stream()
            .mapToInt(entrySet -> calculateBonusGroup(entrySet.getKey(), entrySet.getValue()))
            .sum();
    }

    private void collectBonuses(List<SkillBonus> bonuses, Map<BonusType, List<SkillBonus>> bonusesByType)
    {
        for (SkillBonus bonus : bonuses)
        {
            if (!bonus.isEnabled())
            {
                continue;
            }

            bonusesByType
                    .computeIfAbsent(bonus.getBonusType(), ignored -> new ArrayList<>())
                    .add(bonus);
        }
    }

    private int calculateBonusGroup(BonusType bonusType, List<SkillBonus> typedBonuses)
    {
        return switch (bonusType.getStackingRule())
        {
            case HIGHEST_ONLY -> typedBonuses.stream()
                    .mapToInt(SkillBonus::getValue)
                    .max()
                    .orElse(0);

            case STACKS -> typedBonuses.stream()
                    .mapToInt(SkillBonus::getValue)
                    .sum();

            case STACKS_UNLESS_SAME_SOURCE -> calculateBonusesThatStackUnlessSameSource(typedBonuses);
        };
    }

    private int calculateBonusesThatStackUnlessSameSource(List<SkillBonus> typedBonuses)
    {
        Map<String, Integer> highestBonusBySource = new HashMap<>();

        for (SkillBonus bonus : typedBonuses)
        {
            highestBonusBySource.merge(
                    bonus.getSource(),
                    bonus.getValue(),
                    Math::max
            );
        }

        return highestBonusBySource.values().stream()
                .mapToInt(Integer::intValue)
                .sum();
    }

    private int calculatePenaltyValue(List<SkillPenalty> penalties)
    {
        return penalties.stream()
            .filter(SkillPenalty::isEnabled)
            .mapToInt(SkillPenalty::getValue)
            .sum();
    }

    private int calculateTotalPenalty(SkillEntry entry, SkillAdjustmentSet adjustmentSet)
    {
        return calculatePenaltyValue(entry.getPenalties()) + calculatePenaltyValue(adjustmentSet.getPenalties());
    }

    private int calculateArmorCheckPenalty(SkillType skillType)
    {
        return skillType.isArmorCheckPenaltyApplies() ? armorCheckPenalty : 0;
    }

    private void validateSpecialization(SkillType skillType, String specialization)
    {
        validateSkillType(skillType);

        String normalizedSpecialization = normalize(specialization);
        if (!skillType.isRequiresSpecialization() && !normalizedSpecialization.isEmpty())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        if (skillType.isRequiresSpecialization() && normalizedSpecialization.isEmpty())
        {
            throw new IllegalArgumentException("skillType " + skillType + " requires a specialization");
        }
    }

    private String normalize(String value)
    {
        return value == null ? "" : value.trim();
    }
}
