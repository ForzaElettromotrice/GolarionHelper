package org.golarion.model.character.armorclass;

import lombok.NonNull;
import org.golarion.model.api.ArmorClassData;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.character.modifier.Bonus;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Penalty;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;

public class ArmorClassEntry
{
    private static final EnumSet<BonusType> ALLOWED_BONUS_TYPES = EnumSet.of(
            BonusType.ALCHEMICAL,
            BonusType.ARMOR,
            BonusType.NATURAL_ARMOR,
            BonusType.CIRCUMSTANCE,
            BonusType.INSIGHT,
            BonusType.DEFLECTION,
            BonusType.LUCK,
            BonusType.MORALE,
            BonusType.ENHANCEMENT,
            BonusType.PROFANE,
            BonusType.RACIAL,
            BonusType.SACRED,
            BonusType.DODGE,
            BonusType.SHIELD,
            BonusType.SIZE
    );
    private static final EnumSet<BonusType> TOUCH_EXCLUDED_BONUS_TYPES = EnumSet.of(
            BonusType.ARMOR,
            BonusType.NATURAL_ARMOR,
            BonusType.ENHANCEMENT,
            BonusType.SHIELD
    );
    private static final EnumSet<BonusType> FLAT_FOOTED_EXCLUDED_BONUS_TYPES = EnumSet.of(
            BonusType.DODGE
    );
    private final List<Bonus> bonuses;
    private final List<Penalty> penalties;

    public ArmorClassEntry()
    {
        this.bonuses = new ArrayList<>();
        this.penalties = new ArrayList<>();
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
        if (!ALLOWED_BONUS_TYPES.contains(bonus.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + bonus.getBonusType() + " is not applicable to armor class");
        }

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

    public int getACTotalValue()
    {
        return 10 + getTotalBonus() - getTotalPenalty();
    }

    public int getACTouch()
    {
        return 10 + getTotalBonusExcluding(TOUCH_EXCLUDED_BONUS_TYPES) - getTotalPenalty();
    }

    public int getACFlatFooted()
    {
        return 10 + getTotalBonusExcluding(FLAT_FOOTED_EXCLUDED_BONUS_TYPES) - getTotalPenalty();
    }

    public ArmorClassData toData(int totalValue, int touchValue, int flatFootedValue)
    {
        return new ArmorClassData(totalValue, touchValue, flatFootedValue, getBonuses(), getPenalties());
    }

    private int getTotalBonus()
    {
        return Bonus.calculateTotal(bonuses);
    }

    private int getTotalBonusExcluding(@NonNull EnumSet<BonusType> excludedBonusTypes)
    {
        return Bonus.calculateTotal(
                bonuses.stream()
                        .filter(Bonus::isEnabled)
                        .filter(bonus -> !excludedBonusTypes.contains(bonus.getBonusType()))
                        .toList()
        );
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
