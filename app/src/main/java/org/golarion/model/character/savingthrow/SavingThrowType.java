package org.golarion.model.character.savingthrow;

import lombok.Getter;
import org.golarion.model.character.ability.AbilityType;

@Getter
public enum SavingThrowType
{
    FORTITUDE("Tempra", AbilityType.CONSTITUTION),
    REFLEX("Riflessi", AbilityType.DEXTERITY),
    WILL("Volontà", AbilityType.WISDOM);

    private final String displayName;
    private final AbilityType keyAbility;

    SavingThrowType(String displayName, AbilityType keyAbility)
    {
        this.displayName = displayName;
        this.keyAbility = keyAbility;
    }
}
