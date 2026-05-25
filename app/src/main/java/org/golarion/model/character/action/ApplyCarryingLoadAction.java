package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.api.SpeedData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.equipment.CarryingLoad;
import org.golarion.model.character.modifier.ModifierType;

import java.util.ArrayList;
import java.util.List;

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

        return buildEffectsAction(characterSheet).apply(characterSheet);
    }

    private AddEffectsAction buildEffectsAction(@NonNull CharacterSheet characterSheet)
    {
        return new AddEffectsAction("Ingombro: " + carryingLoad.getDisplayName(), buildEffects(characterSheet));
    }

    private List<AddEffectsAction.Effect> buildEffects(@NonNull CharacterSheet characterSheet)
    {
        List<AddEffectsAction.Effect> effects = new ArrayList<>();

        switch (carryingLoad)
        {
            case MEDIUM -> addLoadEffects(effects, characterSheet, 3, 3, getReducedSpeedUnits(characterSheet.getSpeed()));
            case HEAVY -> addLoadEffects(effects, characterSheet, 1, 6, getReducedSpeedUnits(characterSheet.getSpeed()));
            case OVERLOADED -> addLoadEffects(effects, characterSheet, 0, 6, 1);
            case LIGHT -> throw new IllegalStateException("light load has no effects");
        }

        return effects;
    }

    private void addLoadEffects(
            @NonNull List<AddEffectsAction.Effect> effects,
            @NonNull CharacterSheet characterSheet,
            int maxDexterityBonusLimit,
            int armorCheckPenalty,
            int targetSpeedUnits)
    {
        effects.add(createPenaltyEffect("maxDexLimit", maxDexterityBonusLimit));
        addPenaltyEffect(effects, "ArmorCheckPenalty", armorCheckPenalty);
        addSpeedPenalty(effects, characterSheet, targetSpeedUnits);
    }

    private AddEffectsAction.Effect createPenaltyEffect(@NonNull String target, int penalty)
    {
        return AddEffectsAction.Effect.modifier(
                ModifierType.PENALTY,
                null,
                Integer.toString(penalty),
                target,
                SOURCE,
                carryingLoad.getDisplayName()
        );
    }

    private void addPenaltyEffect(@NonNull List<AddEffectsAction.Effect> effects, @NonNull String target, int penalty)
    {
        if (penalty == 0)
        {
            return;
        }

        effects.add(createPenaltyEffect(target, penalty));
    }

    private void addSpeedPenalty(
            @NonNull List<AddEffectsAction.Effect> effects,
            @NonNull CharacterSheet characterSheet,
            int targetSpeedUnits)
    {
        int currentSpeedUnits = characterSheet.getSpeed().totalUnits();
        int penalty = Math.max(0, currentSpeedUnits - targetSpeedUnits);
        addPenaltyEffect(effects, "speed", penalty);
    }

    private int getReducedSpeedUnits(@NonNull SpeedData speedData)
    {
        return Math.max(1, (speedData.totalUnits() * 2 + 2) / 3);
    }
}
