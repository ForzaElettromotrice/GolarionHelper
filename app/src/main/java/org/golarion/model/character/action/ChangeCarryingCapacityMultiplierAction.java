package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;

public class ChangeCarryingCapacityMultiplierAction implements Action
{
    private final double multiplier;

    public ChangeCarryingCapacityMultiplierAction(double multiplier)
    {
        if (multiplier <= 0)
        {
            throw new IllegalArgumentException("multiplier must be positive");
        }

        this.multiplier = multiplier;
    }

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        characterSheet.changeCarryingCapacityMultiplier(multiplier);
        return () -> characterSheet.changeCarryingCapacityMultiplier(1 / multiplier);
    }
}
