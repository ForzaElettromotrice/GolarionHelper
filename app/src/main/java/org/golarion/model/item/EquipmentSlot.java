package org.golarion.model.item;

import lombok.Getter;

@Getter
public enum EquipmentSlot
{
    HEAD("Testa"),
    HEADBAND("Fronte"),
    EYES("Occhi"),
    SHOULDERS("Spalle"),
    NECK("Collo"),
    CHEST("Torace"),
    BODY("Corpo"),
    ARMOR("Armatura"),
    BELT("Cintura"),
    WRISTS("Polsi"),
    HANDS("Mani"),
    RING("Anello"),
    FEET("Piedi"),
    HAND("Mano");

    private final String displayName;

    EquipmentSlot(String displayName)
    {
        this.displayName = displayName;
    }
}
