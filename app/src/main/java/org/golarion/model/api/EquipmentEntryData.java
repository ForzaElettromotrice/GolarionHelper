package org.golarion.model.api;

import org.golarion.model.character.action.Action;
import org.golarion.model.item.EquipmentSlot;
import org.golarion.model.item.HandUsage;

import java.util.UUID;

public record EquipmentEntryData(
        UUID id,
        String name,
        long unitWeightGrams,
        Integer unitPriceInCopperPieces,
        EquipmentSlot equipmentSlot,
        HandUsage handUsage,
        Action activatedAction,
        String description,
        int quantity
)
{
}
