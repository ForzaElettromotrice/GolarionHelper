package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellComponentType
{
    VERBAL("V"),
    SOMATIC("S"),
    MATERIAL("M"),
    FOCUS("F"),
    DIVINE_FOCUS("FD"),
    MATERIAL_OR_DIVINE_FOCUS("M/FD"),
    FOCUS_OR_DIVINE_FOCUS("F/FD");

    private final String displayName;

    SpellComponentType(String displayName)
    {
        this.displayName = displayName;
    }
}
