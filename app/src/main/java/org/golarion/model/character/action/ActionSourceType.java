package org.golarion.model.character.action;

import lombok.Getter;

@Getter
public enum ActionSourceType
{
    CLASS("Classe"),
    EQUIP("Equipaggiamento"),
    SIZE("Taglia");

    private final String displayName;

    ActionSourceType(String displayName)
    {
        this.displayName = displayName;
    }
}
