package org.golarion.model.character.skill;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.api.SkillData;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.Penalty;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;


public class SkillEntry
{
    private final List<Bonus> bonuses;
    private final List<Penalty> penalties;
    @Getter
    private int ranks;
    @Setter
    @Getter
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

    public List<BonusData> getBonuses()
    {
        return bonuses.stream().map(Bonus::toData).toList();
    }

    public List<PenaltyData> getPenalties()
    {
        return penalties.stream().map(Penalty::toData).toList();
    }

    public void addBonus(Bonus bonus)
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

    public void addPenalty(Penalty penalty)
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

    public int getTotalBonus()
    {
        return Bonus.calculateTotal(bonuses);
    }

    public int getTotalPenalty()
    {
        return penalties.stream().filter(Penalty::isEnabled).mapToInt(Penalty::getValue).sum();
    }

    public SkillData toData(@NonNull SkillType skillType, @NonNull String specialization, int totalModifer)
    {
        return new SkillData(skillType, specialization, classSkill, ranks, totalModifer, getBonuses(), getPenalties());
    }

    private Bonus findBonusById(UUID bonusId)
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

    private Penalty findPenaltyById(UUID penaltyId)
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
