package org.golarion.model.character.equipment;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.character.action.Action;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class Equipment
{
    @Getter
    private final MoneyEntry money;
    private final WeightEntry weightEntry;
    private final List<EquipmentContainerEntry> containers;

    public Equipment()
    {
        this.money = new MoneyEntry();
        this.weightEntry = new WeightEntry();
        this.containers = new ArrayList<>();
    }

    public void setStrengthResolver(@NonNull java.util.function.IntSupplier strengthResolver)
    {
        weightEntry.setStrengthResolver(strengthResolver);
    }

    public EquipmentContainerEntry addContainer(@NonNull String name)
    {
        EquipmentContainerEntry container = new EquipmentContainerEntry(name);
        containers.add(container);
        return container;
    }

    public void removeContainer(@NonNull UUID containerId)
    {
        containers.removeIf(container -> container.getId().equals(containerId));
    }

    public Action addItem(@NonNull UUID containerId, @NonNull EquipmentEntry item)
    {
        if (containsItem(item.getId()))
        {
            throw new IllegalArgumentException("item is already in this equipment");
        }

        findContainer(containerId).addItem(item);
        return weightEntry.changeWeightGrams(item.getTotalWeightGrams());
    }

    public Action moveItem(@NonNull UUID itemId, @NonNull UUID destinationContainerId)
    {
        EquipmentEntry item = removeItemFromAnyContainer(itemId);
        findContainer(destinationContainerId).addItem(item);
        return weightEntry.setCarriedWeightGrams(getTotalWeightGrams());
    }

    public Action removeItem(@NonNull UUID itemId)
    {
        EquipmentEntry item = removeItemFromAnyContainer(itemId);
        return weightEntry.changeWeightGrams(-item.getTotalWeightGrams());
    }

    public long getTotalWeightGrams()
    {
        try
        {
            long totalWeight = money.getTotalWeightGrams();
            for (EquipmentContainerEntry container : containers)
            {
                totalWeight = Math.addExact(totalWeight, container.getTotalWeightGrams());
            }

            return totalWeight;
        } catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("equipment weight is too large", exception);
        }
    }

    public void registerDeltaTargets(@NonNull TargetManager targetManager)
    {
        money.registerDeltaTargets(targetManager);
    }

    public void registerModifierTargets(@NonNull TargetManager targetManager)
    {
        weightEntry.registerTargets(targetManager);
    }

    public void changeCarryingCapacityMultiplier(double multiplier)
    {
        weightEntry.changeCarryingCapacityMultiplier(multiplier);
    }

    private EquipmentContainerEntry findContainer(@NonNull UUID containerId)
    {
        return containers.stream()
                .filter(container -> container.getId().equals(containerId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("container not found"));
    }

    private boolean containsItem(@NonNull UUID itemId)
    {
        return containers.stream()
                .flatMap(container -> container.getItems().stream())
                .anyMatch(item -> item.getId().equals(itemId));
    }

    private EquipmentEntry removeItemFromAnyContainer(@NonNull UUID itemId)
    {
        for (EquipmentContainerEntry container : containers)
        {
            for (EquipmentEntry item : container.getItems())
            {
                if (item.getId().equals(itemId))
                {
                    container.removeItem(itemId);
                    return item;
                }
            }
        }

        throw new IllegalArgumentException("item not found");
    }
}
