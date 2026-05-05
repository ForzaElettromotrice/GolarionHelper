package org.golarion.model.character.savingthrow;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.api.SavingThrowData;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.Penalty;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class SavingThrowEntry
{
    private final List<Bonus> bonuses;
    private final List<Penalty> penalties;
    @Getter
    private int baseValue;

    public SavingThrowEntry()
    {
        this.baseValue = 0;
        this.bonuses = new ArrayList<>();
        this.penalties = new ArrayList<>();
    }

    public void setBaseValue(int baseValue)
    {
        if (baseValue < 0)
        {
            throw new IllegalArgumentException("baseValue must not be negative");
        }

        this.baseValue = baseValue;
    }

    public List<BonusData> getBonuses()
    {
        return bonuses.stream().map(Bonus::toData).toList();
    }

    public List<PenaltyData> getPenalties()
    {
        return penalties.stream().map(Penalty::toData).toList();
    }

    public void addBonus(@NonNull Bonus bonus)
    {
        bonuses.add(bonus);
    }

    public void removeBonus(@NonNull UUID bonusId)
    {
        bonuses.removeIf(bonus -> bonus.getId().equals(bonusId));
    }

    public void setBonusEnabled(@NonNull UUID bonusId, boolean enabled)
    {
        findBonusById(bonusId).setEnabled(enabled);
    }

    public void addPenalty(@NonNull Penalty penalty)
    {
        penalties.add(penalty);
    }

    public void removePenalty(@NonNull UUID penaltyId)
    {
        penalties.removeIf(penalty -> penalty.getId().equals(penaltyId));
    }

    public void setPenaltyEnabled(@NonNull UUID penaltyId, boolean enabled)
    {
        findPenaltyById(penaltyId).setEnabled(enabled);
    }

    public int getTotalValue()
    {
        return baseValue + getTotalBonus() - getTotalPenalty();
    }

    public SavingThrowData toData(@NonNull SavingThrowType savingThrowType, int totalModifier)
    {
        return new SavingThrowData(savingThrowType, baseValue, totalModifier, getBonuses(), getPenalties());
    }

    private int getTotalBonus()
    {
        return Bonus.calculateTotal(bonuses);
    }

    private int getTotalPenalty()
    {
        return penalties.stream().filter(Penalty::isEnabled).mapToInt(Penalty::getValue).sum();
    }

    private Bonus findBonusById(@NonNull UUID bonusId)
    {
        return bonuses.stream()
                .filter(bonus -> bonus.getId().equals(bonusId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("bonusId not found: " + bonusId));
    }

    private Penalty findPenaltyById(@NonNull UUID penaltyId)
    {
        return penalties.stream()
                .filter(penalty -> penalty.getId().equals(penaltyId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("penaltyId not found: " + penaltyId));
    }
}
