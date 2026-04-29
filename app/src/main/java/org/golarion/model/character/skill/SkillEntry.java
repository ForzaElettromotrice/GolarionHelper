package org.golarion.model.character.skill;

import lombok.Getter;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;

@Getter
public class SkillEntry
{
    private final List<SkillBonus> bonuses;
    private final List<SkillPenalty> penalties;
    private int ranks;
    private boolean classSkill;

    public SkillEntry()
    {
        this(0, false);
    }

    public SkillEntry(int ranks, boolean classSkill)
    {
        this.ranks = 0;
        this.classSkill = classSkill;
        this.bonuses = new ArrayList<>();
        this.penalties = new ArrayList<>();

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

    public void setClassSkill(boolean classSkill)
    {
        this.classSkill = classSkill;
    }

    public List<SkillBonus> getBonuses()
    {
        return Collections.unmodifiableList(bonuses);
    }

    public List<SkillPenalty> getPenalties()
    {
        return Collections.unmodifiableList(penalties);
    }

    public void addBonus(SkillBonus bonus)
    {
        if (bonus == null)
        {
            throw new IllegalArgumentException("bonus must not be null");
        }

        bonuses.add(bonus);
    }

    public void removeBonus(UUID bonusId)
    {
        if (bonusId == null)
        {
            throw new IllegalArgumentException("bonusId must not be null");
        }

        bonuses.removeIf(bonus -> bonus.getId().equals(bonusId));
    }

    public void setBonusEnabled(UUID bonusId, boolean enabled)
    {
        findBonusById(bonusId).setEnabled(enabled);
    }

    public void addPenalty(SkillPenalty penalty)
    {
        if (penalty == null)
        {
            throw new IllegalArgumentException("penalty must not be null");
        }

        penalties.add(penalty);
    }

    public void removePenalty(UUID penaltyId)
    {
        if (penaltyId == null)
        {
            throw new IllegalArgumentException("penaltyId must not be null");
        }

        penalties.removeIf(penalty -> penalty.getId().equals(penaltyId));
    }

    public void setPenaltyEnabled(UUID penaltyId, boolean enabled)
    {
        findPenaltyById(penaltyId).setEnabled(enabled);
    }

    private SkillBonus findBonusById(UUID bonusId)
    {
        if (bonusId == null)
        {
            throw new IllegalArgumentException("bonusId must not be null");
        }

        return bonuses.stream()
            .filter(bonus -> bonus.getId().equals(bonusId))
            .findFirst()
            .orElseThrow(() -> new IllegalArgumentException("bonusId not found: " + bonusId));
    }

    private SkillPenalty findPenaltyById(UUID penaltyId)
    {
        if (penaltyId == null)
        {
            throw new IllegalArgumentException("penaltyId must not be null");
        }

        return penalties.stream()
            .filter(penalty -> penalty.getId().equals(penaltyId))
            .findFirst()
            .orElseThrow(() -> new IllegalArgumentException("penaltyId not found: " + penaltyId));
    }
}
