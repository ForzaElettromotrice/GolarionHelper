package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum BloodlineType
{
    ABERRANT("Aberrante"),
    ABYSSAL("Abissale"),
    ACCURSED("Maledetta"),
    AQUATIC("Acquatica"),
    ARCANE("Arcana"),
    CELESTIAL("Celestiale"),
    DESTINED("Predestinata"),
    DRACONIC("Draconica"),
    EARTH("Terrena"),
    ELEMENTAL("Elementale"),
    FEY("Fatata"),
    IMPOSSIBLE("Impossibile"),
    INFERNAL("Infernale"),
    MARID("Marid"),
    ONI("Oni"),
    PHOENIX("Fenice"),
    PESTILENCE("Pestilenza"),
    PROTEAN("Protean"),
    PSYCHIC("Psichica"),
    SERPENTINE("Serpentina"),
    SHADOW("Ombra"),
    STARSOUL("Anima Stellare"),
    STORMBORN("Nata dalla Tempesta"),
    UNDEAD("Non Morta"),
    VERMIN("Parassiti");

    private final String displayName;

    BloodlineType(String displayName)
    {
        this.displayName = displayName;
    }
}
