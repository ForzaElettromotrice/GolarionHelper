package org.golarion.model.item;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.character.action.Action;
import org.golarion.model.character.action.NoAction;

import java.util.Objects;

@Getter
public class ItemDefinition
{
    public static class Builder
    {
        private String name;
        private long weightGrams;
        private Integer priceInCopperPieces;
        private EquipmentSlot equipmentSlot;
        private HandUsage handUsage = HandUsage.NONE;
        private Action equippedAction = NoAction.INSTANCE;
        private Action activatedAction = NoAction.INSTANCE;
        private String description;

        public Builder name(String name)
        {
            this.name = name;
            return this;
        }

        public Builder weightGrams(long weightGrams)
        {
            this.weightGrams = weightGrams;
            return this;
        }

        public Builder priceInCopperPieces(Integer priceInCopperPieces)
        {
            this.priceInCopperPieces = priceInCopperPieces;
            return this;
        }

        public Builder equipmentSlot(EquipmentSlot equipmentSlot)
        {
            this.equipmentSlot = equipmentSlot;
            return this;
        }

        public Builder handUsage(HandUsage handUsage)
        {
            this.handUsage = handUsage == null ? HandUsage.NONE : handUsage;
            return this;
        }

        public Builder equippedAction(Action equippedAction)
        {
            this.equippedAction = equippedAction == null ? NoAction.INSTANCE : equippedAction;
            return this;
        }

        public Builder activatedAction(Action activatedAction)
        {
            this.activatedAction = activatedAction == null ? NoAction.INSTANCE : activatedAction;
            return this;
        }

        public Builder description(String description)
        {
            this.description = description;
            return this;
        }

        public ItemDefinition build()
        {
            return new ItemDefinition(this);
        }
    }

    public static Builder builder()
    {
        return new Builder();
    }

    @NonNull
    private final String name;
    private final long weightGrams;
    private final Integer priceInCopperPieces;
    private final EquipmentSlot equipmentSlot;
    @NonNull
    private final HandUsage handUsage;
    @NonNull
    private final Action equippedAction;
    @NonNull
    private final Action activatedAction;
    private final String description;

    private ItemDefinition(@NonNull Builder builder)
    {
        this.name = normalizeName(builder.name);
        this.weightGrams = validateWeight(builder.weightGrams);
        this.priceInCopperPieces = validatePrice(builder.priceInCopperPieces);
        this.equipmentSlot = builder.equipmentSlot;
        this.handUsage = validateHandUsage(builder.equipmentSlot, builder.handUsage);
        this.equippedAction = builder.equippedAction;
        this.activatedAction = builder.activatedAction;
        this.description = normalizeOptional(builder.description);
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

    private long validateWeight(long weightGrams)
    {
        if (weightGrams < 0)
        {
            throw new IllegalArgumentException("weightGrams must not be negative");
        }

        return weightGrams;
    }

    private Integer validatePrice(Integer priceInCopperPieces)
    {
        if (priceInCopperPieces != null && priceInCopperPieces < 0)
        {
            throw new IllegalArgumentException("priceInCopperPieces must not be negative");
        }

        return priceInCopperPieces;
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
