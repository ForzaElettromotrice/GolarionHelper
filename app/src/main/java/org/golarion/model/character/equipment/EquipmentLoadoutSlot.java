package org.golarion.model.character.equipment;

import lombok.Getter;

@Getter
public enum EquipmentLoadoutSlot
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
    LEFT_RING("Anello Sinistro"),
    RIGHT_RING("Anello Destro"),
    FEET("Piedi"),
    SHIELD("Scudo"),
    MAIN_HAND("Mano Principale"),
    OFF_HAND("Mano Secondaria");

    private final String displayName;

    EquipmentLoadoutSlot(String displayName)
    {
        this.displayName = displayName;
    }
}
