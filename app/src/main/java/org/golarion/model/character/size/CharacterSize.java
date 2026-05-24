package org.golarion.model.character.size;

import lombok.Getter;

@Getter
public enum CharacterSize
{
    FINE("Piccolissima", 8, -8, 16, 8, 0.125),
    DIMINUTIVE("Minuta", 4, -4, 12, 6, 0.25),
    TINY("Minuscola", 2, -2, 8, 4, 0.5),
    SMALL("Piccola", 1, -1, 4, 2, 0.75),
    MEDIUM("Media", 0, 0, 0, 0, 1.0),
    LARGE("Grande", -1, 1, -4, -2, 2.0),
    HUGE("Enorme", -2, 2, -8, -4, 4.0),
    GARGANTUAN("Mastodontica", -4, 4, -12, -6, 8.0),
    COLOSSAL("Colossale", -8, 8, -16, -8, 16.0);

    private final String displayName;
    private final int combatModifier;
    private final int combatManeuverModifier;
    private final int stealthModifier;
    private final int flyModifier;
    private final double carryingCapacityMultiplier;

    CharacterSize(String displayName, int combatModifier, int combatManeuverModifier, int stealthModifier, int flyModifier, double carryingCapacityMultiplier)
    {
        this.displayName = displayName;
        this.combatModifier = combatModifier;
        this.combatManeuverModifier = combatManeuverModifier;
        this.stealthModifier = stealthModifier;
        this.flyModifier = flyModifier;
        this.carryingCapacityMultiplier = carryingCapacityMultiplier;
    }
}
