package org.golarion.model.character.equipment;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.item.EquipmentSlot;
import org.golarion.model.item.HandUsage;
import org.golarion.model.item.ItemDefinition;

import java.util.Objects;
import java.util.UUID;

@Getter
public class EquipmentEntry
{
    public static EquipmentEntry fromDefinition(@NonNull ItemDefinition itemDefinition)
    {
        EquipmentEntry equipmentEntry = new EquipmentEntry(itemDefinition.getName());
        equipmentEntry.setUnitWeightGrams(itemDefinition.getWeightGrams());
        equipmentEntry.setUnitPriceInCopperPieces(itemDefinition.getPriceInCopperPieces());
        equipmentEntry.setEquipmentSlot(itemDefinition.getEquipmentSlot());
        equipmentEntry.setHandUsage(itemDefinition.getHandUsage());
        equipmentEntry.setDescription(itemDefinition.getDescription());
        return equipmentEntry;
    }
    private final UUID id;
    @NonNull
    private String name;
    private long unitWeightGrams;
    private Integer unitPriceInCopperPieces;
    private EquipmentSlot equipmentSlot;
    @NonNull
    private HandUsage handUsage;
    private String description;
    private int quantity;

    public EquipmentEntry(@NonNull String name)
    {
        this.id = UUID.randomUUID();
        this.name = normalizeName(name);
        this.handUsage = HandUsage.NONE;
        this.quantity = 1;
    }

    public void setName(@NonNull String name)
    {
        this.name = normalizeName(name);
    }

    public void setUnitWeightGrams(long unitWeightGrams)
    {
        if (unitWeightGrams < 0)
        {
            throw new IllegalArgumentException("unitWeightGrams must not be negative");
        }

        this.unitWeightGrams = unitWeightGrams;
    }

    public void setUnitPriceInCopperPieces(Integer unitPriceInCopperPieces)
    {
        if (unitPriceInCopperPieces != null && unitPriceInCopperPieces < 0)
        {
            throw new IllegalArgumentException("unitPriceInCopperPieces must not be negative");
        }

        this.unitPriceInCopperPieces = unitPriceInCopperPieces;
    }

    public void setEquipmentSlot(EquipmentSlot equipmentSlot)
    {
        this.equipmentSlot = equipmentSlot;
        if (!canUseHands(equipmentSlot))
        {
            this.handUsage = HandUsage.NONE;
        }
    }

    public void setHandUsage(HandUsage handUsage)
    {
        this.handUsage = validateHandUsage(equipmentSlot, handUsage);
    }

    public void setDescription(String description)
    {
        this.description = normalizeOptional(description);
    }

    public void setQuantity(int quantity)
    {
        if (quantity < 1)
        {
            throw new IllegalArgumentException("quantity must be positive");
        }

        this.quantity = quantity;
    }

    public long getTotalWeightGrams()
    {
        try
        {
            return Math.multiplyExact(unitWeightGrams, quantity);
        } catch (ArithmeticException exception)
        {
            throw new IllegalArgumentException("equipment weight is too large", exception);
        }
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

    private String normalizeOptional(String value)
    {
        if (value == null)
        {
            return null;
        }

        String normalizedValue = value.trim();
        return normalizedValue.isBlank() ? null : normalizedValue;
    }

    private HandUsage validateHandUsage(EquipmentSlot equipmentSlot, HandUsage handUsage)
    {
        HandUsage normalizedHandUsage = handUsage == null ? HandUsage.NONE : handUsage;
        if (!canUseHands(equipmentSlot) && normalizedHandUsage != HandUsage.NONE)
        {
            throw new IllegalArgumentException("handUsage can be set only for hand or shield equipment");
        }

        return normalizedHandUsage;
    }

    private boolean canUseHands(EquipmentSlot equipmentSlot)
    {
        return equipmentSlot == EquipmentSlot.HAND || equipmentSlot == EquipmentSlot.SHIELD;
    }
}
