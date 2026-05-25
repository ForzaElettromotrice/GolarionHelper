package org.golarion.model.character.equipment;

import lombok.Getter;
import lombok.NonNull;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.UUID;

public class EquipmentContainerEntry
{
    @Getter
    private final UUID id;
    @Getter
    @NonNull
    private final String name;
    private final List<EquipmentEntry> items;
    @Getter
    private boolean contentWeightIgnored;

    public EquipmentContainerEntry(@NonNull String name)
    {
        this.id = UUID.randomUUID();
        this.name = normalizeName(name);
        this.items = new ArrayList<>();
    }

    public List<EquipmentEntry> getItems()
    {
        return List.copyOf(items);
    }

    public void addItem(@NonNull EquipmentEntry item)
    {
        if (containsItem(item.getId()))
        {
            throw new IllegalArgumentException("item is already in this container");
        }

        items.add(item);
    }

    public void removeItem(@NonNull UUID itemId)
    {
        items.removeIf(item -> item.getId().equals(itemId));
    }

    public void setContentWeightIgnored(boolean contentWeightIgnored)
    {
        this.contentWeightIgnored = contentWeightIgnored;
    }

    public long getTotalWeightGrams()
    {
        return contentWeightIgnored ? 0 : getContentsWeightGrams();
    }

    public long getContentsWeightGrams()
    {
        try
        {
            long totalWeight = 0;
            for (EquipmentEntry item : items)
            {
                totalWeight = Math.addExact(totalWeight, item.getTotalWeightGrams());
            }

            return totalWeight;
        } catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("container contents weight is too large", exception);
        }
    }

    private boolean containsItem(@NonNull UUID itemId)
    {
        return items.stream().anyMatch(item -> item.getId().equals(itemId));
    }

    private String normalizeName(String value)
    {
        String normalizedValue = Objects.requireNonNull(value, "name must not be null").trim();
        if (normalizedValue.isBlank())
        {
            throw new IllegalArgumentException("name must not be blank");
        }

        return normalizedValue;
    }
}
