package org.golarion.model.character.equipment;

import lombok.NonNull;
import org.golarion.model.item.EquipmentSlot;
import org.golarion.model.item.HandUsage;

import java.util.EnumMap;
import java.util.List;
import java.util.UUID;

public class EquipmentLoadout
{
    private final EnumMap<EquipmentLoadoutSlot, EquipmentEntry> equippedItems;

    public EquipmentLoadout()
    {
        this.equippedItems = new EnumMap<>(EquipmentLoadoutSlot.class);
    }

    public void equip(@NonNull EquipmentEntry item, @NonNull EquipmentLoadoutSlot slot)
    {
        EquipmentSlot equipmentSlot = requireEquipmentSlot(item);
        occupy(item, getOccupiedSlots(equipmentSlot, item.getHandUsage(), slot));
    }

    public void unequip(@NonNull UUID itemId)
    {
        if (!isEquipped(itemId))
        {
            throw new IllegalArgumentException("item is not equipped");
        }

        equippedItems.entrySet().removeIf(entry -> entry.getValue().getId().equals(itemId));
    }

    public boolean isEquipped(@NonNull UUID itemId)
    {
        return equippedItems.values().stream().anyMatch(item -> item.getId().equals(itemId));
    }

    private void occupy(@NonNull EquipmentEntry item, @NonNull List<EquipmentLoadoutSlot> slots)
    {
        if (isEquipped(item.getId()))
        {
            throw new IllegalArgumentException("item is already equipped");
        }

        for (EquipmentLoadoutSlot slot : slots)
        {
            if (equippedItems.containsKey(slot))
            {
                throw new IllegalStateException("slot is already occupied: " + slot);
            }
        }

        for (EquipmentLoadoutSlot slot : slots)
        {
            equippedItems.put(slot, item);
        }
    }

    private List<EquipmentLoadoutSlot> getOccupiedSlots(
            @NonNull EquipmentSlot equipmentSlot,
            @NonNull HandUsage handUsage,
            @NonNull EquipmentLoadoutSlot targetSlot)
    {
        return switch (equipmentSlot)
        {
            case HAND -> getHandOccupiedSlots(handUsage, targetSlot);
            case SHIELD -> getShieldOccupiedSlots(handUsage, targetSlot);
            default ->
            {
                if (!isCompatibleSlot(equipmentSlot, targetSlot))
                {
                    throw new IllegalArgumentException("item cannot be equipped in slot " + targetSlot);
                }

                yield List.of(targetSlot);
            }
        };
    }

    private List<EquipmentLoadoutSlot> getHandOccupiedSlots(
            @NonNull HandUsage handUsage,
            @NonNull EquipmentLoadoutSlot targetSlot)
    {
        validateHandSlot(targetSlot);

        return switch (handUsage)
        {
            case ONE_HAND -> List.of(targetSlot);
            case TWO_HANDS -> List.of(EquipmentLoadoutSlot.MAIN_HAND, EquipmentLoadoutSlot.OFF_HAND);
            case NONE -> throw new IllegalArgumentException("hand equipment must use one or two hands");
        };
    }

    private List<EquipmentLoadoutSlot> getShieldOccupiedSlots(
            @NonNull HandUsage handUsage,
            @NonNull EquipmentLoadoutSlot targetSlot)
    {
        return switch (handUsage)
        {
            case NONE ->
            {
                if (targetSlot != EquipmentLoadoutSlot.SHIELD)
                {
                    throw new IllegalArgumentException("shield equipment without hand usage must be equipped in shield slot");
                }

                yield List.of(EquipmentLoadoutSlot.SHIELD);
            }
            case ONE_HAND ->
            {
                validateHandSlot(targetSlot);
                yield List.of(EquipmentLoadoutSlot.SHIELD, targetSlot);
            }
            case TWO_HANDS -> throw new IllegalArgumentException("shield equipment cannot use two hands");
        };
    }

    private EquipmentSlot requireEquipmentSlot(@NonNull EquipmentEntry item)
    {
        EquipmentSlot equipmentSlot = item.getEquipmentSlot();
        if (equipmentSlot == null)
        {
            throw new IllegalArgumentException("item is not equippable");
        }

        return equipmentSlot;
    }

    private void validateHandSlot(@NonNull EquipmentLoadoutSlot slot)
    {
        if (slot != EquipmentLoadoutSlot.MAIN_HAND && slot != EquipmentLoadoutSlot.OFF_HAND)
        {
            throw new IllegalArgumentException("slot must be a hand slot");
        }
    }

    private boolean isCompatibleSlot(@NonNull EquipmentSlot equipmentSlot, @NonNull EquipmentLoadoutSlot loadoutSlot)
    {
        return switch (equipmentSlot)
        {
            case HEAD -> loadoutSlot == EquipmentLoadoutSlot.HEAD;
            case HEADBAND -> loadoutSlot == EquipmentLoadoutSlot.HEADBAND;
            case EYES -> loadoutSlot == EquipmentLoadoutSlot.EYES;
            case SHOULDERS -> loadoutSlot == EquipmentLoadoutSlot.SHOULDERS;
            case NECK -> loadoutSlot == EquipmentLoadoutSlot.NECK;
            case CHEST -> loadoutSlot == EquipmentLoadoutSlot.CHEST;
            case BODY -> loadoutSlot == EquipmentLoadoutSlot.BODY;
            case ARMOR -> loadoutSlot == EquipmentLoadoutSlot.ARMOR;
            case BELT -> loadoutSlot == EquipmentLoadoutSlot.BELT;
            case WRISTS -> loadoutSlot == EquipmentLoadoutSlot.WRISTS;
            case HANDS -> loadoutSlot == EquipmentLoadoutSlot.HANDS;
            case RING ->
                    loadoutSlot == EquipmentLoadoutSlot.LEFT_RING || loadoutSlot == EquipmentLoadoutSlot.RIGHT_RING;
            case FEET -> loadoutSlot == EquipmentLoadoutSlot.FEET;
            case SHIELD -> loadoutSlot == EquipmentLoadoutSlot.SHIELD;
            case HAND -> loadoutSlot == EquipmentLoadoutSlot.MAIN_HAND || loadoutSlot == EquipmentLoadoutSlot.OFF_HAND;
        };
    }
}
