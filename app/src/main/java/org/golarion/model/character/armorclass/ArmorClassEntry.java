package org.golarion.model.character.armorclass;

import lombok.NonNull;
import org.golarion.model.api.ArmorClassData;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.Modifiable;
import org.golarion.model.character.modifier.Modifier;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.UUID;

public class ArmorClassEntry implements Modifiable
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
    private final List<Modifier> modifiers;

    public ArmorClassEntry()
    {
        this.modifiers = new ArrayList<>();
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
                modifiers.stream().map(Modifier::toData).toList()
        );
    }

    public int getTotalValue(int abilityModifier)
    {
        return 10 + abilityModifier + Modifier.calculateTotal(modifiers);
    }

    public int getTouchValue(int abilityModifier)
    {
        return 10 + abilityModifier + getTotalModifierExcluding(TOUCH_EXCLUDED_BONUS_TYPES);
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
}
