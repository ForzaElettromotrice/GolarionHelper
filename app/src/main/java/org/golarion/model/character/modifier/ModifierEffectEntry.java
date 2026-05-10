package org.golarion.model.character.modifier;

import lombok.NonNull;
import org.golarion.model.api.EffectEntryData;

import java.util.UUID;

public class ModifierEffectEntry implements EffectEntry
{
    private final String targetName;
    private final ModifierTarget target;
    private final Modifier modifier;

    public ModifierEffectEntry(@NonNull String targetName, @NonNull ModifierTarget target, @NonNull Modifier modifier)
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
        modifier.setEnabled(enabled);
    }

    @Override
    public void remove()
    {
        target.removeModifier(modifier.getId());
    }

    @Override
    public EffectEntryData toData()
    {
        return new EffectEntryData(
                modifier.getId(),
                EffectEntryType.MODIFIER,
                targetName,
                modifier.getType(),
                modifier.getBonusType(),
                modifier.getSource(),
                modifier.isEnabled(),
                modifier.getDescription(),
                modifier.getExpression().getExpression()
        );
    }
}
