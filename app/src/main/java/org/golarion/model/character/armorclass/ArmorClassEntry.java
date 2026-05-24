package org.golarion.model.character.armorclass;

import lombok.NonNull;
import org.golarion.model.api.ArmorClassData;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;

public class ArmorClassEntry implements ModifierTarget
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
    private static final int NO_MAX_DEXTERITY_BONUS_LIMIT = -1;
    private final List<Modifier> modifiers;
    private final MaxDexterityBonusModifiers maxDexterityBonusModifiers;
    private final MaxDexterityBonusLimitModifiers maxDexterityBonusLimitModifiers;
    private int maxDexterityBonus;

    public ArmorClassEntry()
    {
        this.modifiers = new ArrayList<>();
        this.maxDexterityBonusModifiers = new MaxDexterityBonusModifiers();
        this.maxDexterityBonusLimitModifiers = new MaxDexterityBonusLimitModifiers();
        this.maxDexterityBonus = NO_MAX_DEXTERITY_BONUS_LIMIT;
    }

    @Override
    public void addModifier(@NonNull Modifier modifier)
    {
        if (modifier.getBonusType() != null && !ALLOWED_BONUS_TYPES.contains(modifier.getBonusType()))
        {
            throw new IllegalArgumentException("bonusType " + modifier.getBonusType() + " is not applicable to armor class");
        }

        modifiers.add(modifier);
    }

    @Override
    public void removeModifier(@NonNull UUID modifierId)
    {
        modifiers.removeIf(bonus -> bonus.getId().equals(modifierId));
    }

    public ArmorClassData toData(int abilityModifier)
    {
        return new ArmorClassData(
                getTotalValue(abilityModifier),
                getTouchValue(abilityModifier),
                getFlatFootedValue(),
                getMaxDexterityBonusDataValue(),
                getMaxDexterityBonusModifierData(),
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    public void registerTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget("armorClass", this);
        targetManager.registerModifierTarget("maxDex", maxDexterityBonusModifiers);
        targetManager.registerModifierTarget("maxDexLimit", maxDexterityBonusLimitModifiers);
    }

    public void setMaxDexterityBonus(int maxDexterityBonus)
    {
        if (maxDexterityBonus < NO_MAX_DEXTERITY_BONUS_LIMIT)
        {
            throw new IllegalArgumentException("maxDexterityBonus must be -1 or greater");
        }

        this.maxDexterityBonus = maxDexterityBonus;
    }

    public int getTotalValue(int abilityModifier)
    {
        return 10 + getLimitedAbilityModifier(abilityModifier) + Modifier.calculateTotal(modifiers);
    }

    public int getTouchValue(int abilityModifier)
    {
        return 10 + getLimitedAbilityModifier(abilityModifier) + getTotalModifierExcluding(TOUCH_EXCLUDED_BONUS_TYPES);
    }

    public int getFlatFootedValue()
    {
        return 10 + getTotalModifierExcluding(FLAT_FOOTED_EXCLUDED_BONUS_TYPES);
    }

    private int getTotalModifierExcluding(@NonNull EnumSet<BonusType> excludedBonusTypes)
    {
        return Modifier.calculateTotal(
                modifiers.stream()
                        .filter(Modifier::isEnabled)
                        .filter(modifier -> modifier.getBonusType() == null || !excludedBonusTypes.contains(modifier.getBonusType()))
                        .toList()
        );
    }

    private int getLimitedAbilityModifier(int abilityModifier)
    {
        Integer totalMaxDexterityBonus = getMaxDexterityBonusDataValue();
        if (totalMaxDexterityBonus == null)
        {
            return abilityModifier;
        }

        return Math.min(abilityModifier, totalMaxDexterityBonus);
    }

    private int getTotalMaxDexterityBonus()
    {
        Integer limit = null;
        if (maxDexterityBonus != NO_MAX_DEXTERITY_BONUS_LIMIT)
        {
            limit = Math.max(0, maxDexterityBonus + maxDexterityBonusModifiers.getTotalValue());
        }

        Integer externalLimit = maxDexterityBonusLimitModifiers.getLowestLimit();
        if (externalLimit != null)
        {
            limit = limit == null ? externalLimit : Math.min(limit, externalLimit);
        }

        return limit == null ? NO_MAX_DEXTERITY_BONUS_LIMIT : limit;
    }

    private Integer getMaxDexterityBonusDataValue()
    {
        int totalMaxDexterityBonus = getTotalMaxDexterityBonus();
        return totalMaxDexterityBonus == NO_MAX_DEXTERITY_BONUS_LIMIT ? null : totalMaxDexterityBonus;
    }

    private List<org.golarion.model.api.ModifierData> getMaxDexterityBonusModifierData()
    {
        List<org.golarion.model.api.ModifierData> data = new ArrayList<>();
        data.addAll(maxDexterityBonusModifiers.toData());
        data.addAll(maxDexterityBonusLimitModifiers.toData());
        return data;
    }
}
