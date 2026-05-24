package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.api.SpeedData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.equipment.CarryingLoad;
import org.golarion.model.character.modifier.ModifierType;

import java.util.UUID;

public class ApplyCarryingLoadAction implements Action
{
    private static final String SOURCE = "Ingombro";

    private final CarryingLoad carryingLoad;

    public ApplyCarryingLoadAction(@NonNull CarryingLoad carryingLoad)
    {
        this.carryingLoad = carryingLoad;
    }

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        if (carryingLoad == CarryingLoad.LIGHT)
        {
            return NoAction.INSTANCE;
        }

        UUID effectGroupId = characterSheet.createEffectGroup("Ingombro: " + carryingLoad.getDisplayName());
        try
        {
            applyEffects(characterSheet, effectGroupId);
        }
        catch (RuntimeException exception)
        {
            characterSheet.removeEffectGroup(effectGroupId);
            throw exception;
        }

        return () -> characterSheet.removeEffectGroup(effectGroupId);
    }

    private void applyEffects(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId)
    {
        switch (carryingLoad)
        {
            case MEDIUM -> applyLoadEffects(characterSheet, effectGroupId, 3, 3, getReducedSpeedUnits(characterSheet.getSpeed()));
            case HEAVY -> applyLoadEffects(characterSheet, effectGroupId, 1, 6, getReducedSpeedUnits(characterSheet.getSpeed()));
            case OVERLOADED -> applyLoadEffects(characterSheet, effectGroupId, 0, 6, 1);
            case LIGHT -> throw new IllegalStateException("light load has no effects");
        }
    }

    private void applyLoadEffects(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId, int maxDexterityBonusLimit, int armorCheckPenalty, int targetSpeedUnits)
    {
        addLimitEffect(characterSheet, effectGroupId, "maxDexLimit", maxDexterityBonusLimit);
        addPenaltyEffect(characterSheet, effectGroupId, "ArmorCheckPenalty", armorCheckPenalty);
        addSpeedPenalty(characterSheet, effectGroupId, targetSpeedUnits);
    }

    private void addLimitEffect(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId, @NonNull String target, int limit)
    {
        characterSheet.addEffect(
                effectGroupId,
                ModifierType.PENALTY,
                null,
                Integer.toString(limit),
                target,
                SOURCE,
                carryingLoad.getDisplayName()
        );
    }

    private void addPenaltyEffect(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId, @NonNull String target, int penalty)
    {
        if (penalty == 0)
        {
            return;
        }

        characterSheet.addEffect(
                effectGroupId,
                ModifierType.PENALTY,
                null,
                Integer.toString(penalty),
                target,
                SOURCE,
                carryingLoad.getDisplayName()
        );
    }

    private void addSpeedPenalty(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId, int targetSpeedUnits)
    {
        int currentSpeedUnits = characterSheet.getSpeed().totalUnits();
        int penalty = Math.max(0, currentSpeedUnits - targetSpeedUnits);
        addPenaltyEffect(characterSheet, effectGroupId, "speed", penalty);
    }

    private int getReducedSpeedUnits(@NonNull SpeedData speedData)
    {
        return Math.max(1, (speedData.totalUnits() * 2 + 2) / 3);
    }
}
