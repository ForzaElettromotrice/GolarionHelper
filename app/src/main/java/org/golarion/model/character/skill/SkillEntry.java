package org.golarion.model.character.skill;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.SkillData;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Modifier;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;


public class SkillEntry
{
    private static final EnumSet<BonusType> ALLOWED_BONUS_TYPES = EnumSet.of(
            BonusType.ALCHEMICAL,
            BonusType.CIRCUMSTANCE,
            BonusType.INSIGHT,
            BonusType.COMPETENCE,
            BonusType.LUCK,
            BonusType.MORALE,
            BonusType.ENHANCEMENT,
            BonusType.PROFANE,
            BonusType.RACIAL,
            BonusType.SACRED,
            BonusType.SIZE
    );
    private final List<Modifier> modifiers;
    private int ranks;
    @Setter
    @Getter
    private boolean classSkill;

    public SkillEntry()
    {
        this.ranks = 0;
        this.classSkill = false;
        this.modifiers = new ArrayList<>();

        setRanks(ranks);
    }

    public void setRanks(int ranks)
    {
        if (ranks < 0)
        {
            throw new IllegalArgumentException("ranks must not be negative");
        }

        this.ranks = ranks;
    }

    public void addModifier(@NonNull Modifier modifier)
    {
        if (modifier.getBonusType() != null && !ALLOWED_BONUS_TYPES.contains(modifier.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + modifier.getBonusType() + " is not applicable to a skill");
        }

        modifiers.add(modifier);
    }

    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(bonus -> bonus.getId().equals(modifierId));
    }

    public void setModifierEnabled(@NonNull UUID modifierId, boolean enabled)
    {
        modifiers.stream()
                .filter(modifier -> modifier.getId().equals(modifierId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("modifierId not found: " + modifierId))
                .setEnabled(enabled);
    }

    public SkillData toData(@NonNull SkillType skillType, @NonNull String specialization, int abilityModifier)
    {
        return new SkillData(
                skillType,
                specialization,
                classSkill,
                ranks,
                getTotalValue(abilityModifier),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    private int getTotalValue(int abilityModifier)
    {
        return abilityModifier
                + ranks
                + (classSkill && ranks > 0 ? 3 : 0)
                + Modifier.calculateTotal(modifiers);
    }
}
