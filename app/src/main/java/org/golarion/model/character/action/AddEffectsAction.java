package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.List;
import java.util.UUID;

public class AddEffectsAction implements Action
{
    public record Effect(
            @NonNull ModifierType modifierType,
            BonusType bonusType,
            @NonNull String expression,
            @NonNull String target,
            @NonNull String source,
            @NonNull String description,
            String condition)
    {
        public static Effect modifier(
                @NonNull ModifierType modifierType,
                BonusType bonusType,
                @NonNull String expression,
                @NonNull String target,
                @NonNull String source,
                @NonNull String description)
        {
            return new Effect(modifierType, bonusType, expression, target, source, description, null);
        }

        public static Effect conditionalModifier(
                @NonNull ModifierType modifierType,
                BonusType bonusType,
                @NonNull String expression,
                @NonNull String target,
                @NonNull String source,
                @NonNull String description,
                @NonNull String condition)
        {
            return new Effect(modifierType, bonusType, expression, target, source, description, condition);
        }

        public boolean isConditional()
        {
            return condition != null && !condition.trim().isEmpty();
        }
    }

    private final String effectGroupName;
    private final List<Effect> effects;

    public AddEffectsAction(@NonNull String effectGroupName, @NonNull List<Effect> effects)
    {
        if (effects.isEmpty())
        {
            throw new IllegalArgumentException("effects must not be empty");
        }

        this.effectGroupName = effectGroupName;
        this.effects = List.copyOf(effects);
    }

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        UUID effectGroupId = characterSheet.createEffectGroup(effectGroupName);
        try
        {
            for (Effect effect : effects)
            {
                addEffect(characterSheet, effectGroupId, effect);
            }
        }
        catch (RuntimeException exception)
        {
            characterSheet.removeEffectGroup(effectGroupId);
            throw exception;
        }

        return () -> characterSheet.removeEffectGroup(effectGroupId);
    }

    private void addEffect(
            @NonNull CharacterSheet characterSheet,
            @NonNull UUID effectGroupId,
            @NonNull Effect effect)
    {
        if (effect.isConditional())
        {
            characterSheet.addConditionalEffect(
                    effectGroupId,
                    effect.modifierType(),
                    effect.bonusType(),
                    effect.expression(),
                    effect.target(),
                    effect.source(),
                    effect.description(),
                    effect.condition()
            );
            return;
        }

        characterSheet.addEffect(
                effectGroupId,
                effect.modifierType(),
                effect.bonusType(),
                effect.expression(),
                effect.target(),
                effect.source(),
                effect.description()
        );
    }
}
