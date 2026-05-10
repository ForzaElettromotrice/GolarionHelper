package org.golarion.model.character.modifier;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.EffectGroupData;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class EffectGroup
{
    @Getter
    private final UUID id;
    private final List<EffectEntry> entries;
    private String name;

    public EffectGroup(@NonNull String name)
    {
        this.id = UUID.randomUUID();
        this.entries = new ArrayList<>();
        setName(name);
    }

    public UUID addEntry(@NonNull EffectEntry entry)
    {
        entries.add(entry);
        return entry.getId();
    }

    public void removeEffect(@NonNull UUID effectId)
    {
        EffectEntry entry = entries.stream()
                .filter(currentEntry -> currentEntry.getId().equals(effectId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("effectId not found: " + effectId));

        entry.remove();
        entries.remove(entry);
    }

    public void removeAllEffects()
    {
        List<EffectEntry> entriesToRemove = new ArrayList<>(entries);
        for (EffectEntry entry : entriesToRemove)
        {
            entry.remove();
        }
        entries.clear();
    }

    public void setEnabled(boolean enabled)
    {
        entries.forEach(entry -> entry.setEnabled(enabled));
    }

    public void setName(@NonNull String name)
    {
        if (name.trim().isEmpty())
        {
            throw new IllegalArgumentException("name must not be blank");
        }

        this.name = name.trim();
    }

    public EffectGroupData toData()
    {
        return new EffectGroupData(id, name, entries.stream().map(EffectEntry::toData).toList());
    }
}
