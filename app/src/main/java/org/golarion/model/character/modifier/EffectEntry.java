package org.golarion.model.character.modifier;

import lombok.NonNull;
import org.golarion.model.api.EffectEntryData;

import java.util.UUID;

public interface EffectEntry
{
    @NonNull
    UUID getId();

    void setEnabled(boolean enabled);

    void remove();

    EffectEntryData toData();
}
