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
    private final EquipmentLoadout loadout;
    private final List<EquipmentContainerEntry> containers;

    public Equipment()
    {
        this.money = new MoneyEntry();
        this.weightEntry = new WeightEntry();
        this.loadout = new EquipmentLoadout();
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

    public Action removeContainer(@NonNull UUID containerId)
    {
        containers.removeIf(container -> container.getId().equals(containerId));
        return refreshCarriedWeight();
    }

    public Action setContainerContentWeightIgnored(@NonNull UUID containerId, boolean contentWeightIgnored)
    {
        findContainer(containerId).setContentWeightIgnored(contentWeightIgnored);
        return refreshCarriedWeight();
    }

    public Action addItem(@NonNull UUID containerId, @NonNull EquipmentEntry item)
    {
        if (containsItem(item.getId()))
        {
            throw new IllegalArgumentException("item is already in this equipment");
        }

        findContainer(containerId).addItem(item);
        return refreshCarriedWeight();
    }

    public Action moveItem(@NonNull UUID itemId, @NonNull UUID destinationContainerId)
    {
        EquipmentEntry item = removeItemFromAnyContainer(itemId);
        findContainer(destinationContainerId).addItem(item);
        return refreshCarriedWeight();
    }

    public Action removeItem(@NonNull UUID itemId)
    {
        removeItemFromAnyContainer(itemId);
        return refreshCarriedWeight();
    }

    public Action equipItem(@NonNull UUID itemId, @NonNull EquipmentLoadoutSlot slot)
    {
        EquipmentContainerEntry sourceContainer = findContainerContainingItem(itemId);
        EquipmentEntry item = findItem(sourceContainer, itemId);
        Action action = loadout.equip(item, slot);
        sourceContainer.removeItem(itemId);
        return action;
    }

    public void unequipItem(@NonNull UUID itemId, @NonNull UUID destinationContainerId)
    {
        EquipmentContainerEntry destinationContainer = findContainer(destinationContainerId);
        EquipmentEntry item = loadout.unequip(itemId);
        destinationContainer.addItem(item);
    }

    public Action getActivatedAction(@NonNull UUID itemId)
    {
        EquipmentEntry containerItem = findItemInContainers(itemId);
        if (containerItem != null)
        {
            return containerItem.getActivatedAction();
        }

        if (!loadout.isEquipped(itemId))
        {
            throw new IllegalArgumentException("item not found");
        }

        return loadout.getActivatedAction(itemId);
    }

    public Action refreshCarriedWeight()
    {
        return weightEntry.setCarriedWeightGrams(getTotalWeightGrams());
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
            totalWeight = Math.addExact(totalWeight, loadout.getTotalWeightGrams());

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
                .anyMatch(item -> item.getId().equals(itemId))
                || loadout.isEquipped(itemId);
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

    private EquipmentEntry findItemInContainers(@NonNull UUID itemId)
    {
        for (EquipmentContainerEntry container : containers)
        {
            for (EquipmentEntry item : container.getItems())
            {
                if (item.getId().equals(itemId))
                {
                    return item;
                }
            }
        }

        return null;
    }

    private EquipmentContainerEntry findContainerContainingItem(@NonNull UUID itemId)
    {
        return containers.stream()
                .filter(container -> container.getItems().stream().anyMatch(item -> item.getId().equals(itemId)))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("item not found"));
    }

    private EquipmentEntry findItem(@NonNull EquipmentContainerEntry container, @NonNull UUID itemId)
    {
        return container.getItems().stream()
                .filter(item -> item.getId().equals(itemId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("item not found"));
    }
}
