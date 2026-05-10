package org.golarion.model.character.modifier;

import lombok.NonNull;
import org.golarion.model.api.EffectEntryData;

import java.util.UUID;

public class DeltaEffectEntry implements EffectEntry
{
    private final UUID id;
    private final String targetName;
    private final DeltaTarget target;
    private final String source;
    private final String description;
    private final Expression expression;
    private boolean enabled;
    private int appliedValue;

    public DeltaEffectEntry(
            @NonNull String targetName,
            @NonNull DeltaTarget target,
            @NonNull String source,
            @NonNull String description,
            @NonNull Expression expression
    )
    {
        this.id = UUID.randomUUID();
        this.targetName = targetName;
        this.target = target;
        this.source = source;
        this.description = description;
        this.expression = expression;
        this.enabled = false;
        this.appliedValue = 0;

        setEnabled(true);
    }

    @Override
    public @NonNull UUID getId()
    {
        return id;
    }

    @Override
    public void setEnabled(boolean enabled)
    {
        if (this.enabled == enabled)
        {
            return;
        }

        if (enabled)
        {
            appliedValue = expression.getValue();
            target.changeValue(appliedValue);
            this.enabled = true;
            return;
        }

        target.changeValue(-appliedValue);
        appliedValue = 0;
        this.enabled = false;
    }

    @Override
    public void remove()
    {
        setEnabled(false);
    }

    @Override
    public EffectEntryData toData()
    {
        return new EffectEntryData(
                id,
                EffectEntryType.DELTA,
                targetName,
                null,
                null,
                source,
                enabled,
                description,
                expression.getExpression()
        );
    }
}
