package org.golarion.model.character.modifier;

import lombok.NonNull;
import org.golarion.model.api.EffectEntryData;

import java.util.UUID;

public class ConditionalModifierEffectEntry implements EffectEntry
{
    private final String targetName;
    private final ModifierTarget target;
    private final ConditionalModifier modifier;

    public ConditionalModifierEffectEntry(
            @NonNull String targetName,
            @NonNull ModifierTarget target,
            @NonNull ConditionalModifier modifier)
    {
        this.targetName = targetName;
        this.target = target;
        this.modifier = modifier;
    }

    @Override
    public @NonNull UUID getId()
    {
        return modifier.getId();
    }

    @Override
    public void setEnabled(boolean enabled)
    {
    }

    @Override
    public void remove()
    {
        target.removeConditionalModifier(modifier.getId());
    }

    @Override
    public EffectEntryData toData()
    {
        return new EffectEntryData(
                modifier.getId(),
                EffectEntryType.CONDITIONAL_MODIFIER,
                targetName,
                modifier.getType(),
                modifier.getBonusType(),
                modifier.getSource(),
                true,
                modifier.getDescription(),
                modifier.getExpression().getExpression(),
                modifier.getCondition()
        );
    }
}
