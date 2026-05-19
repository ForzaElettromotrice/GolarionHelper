package org.golarion.model.spell;

import lombok.Getter;

@Getter
public enum SpellSubschool
{
    CALLING("Richiamo", SpellSchool.CONJURATION),
    CHARM("Charme", SpellSchool.ENCHANTMENT),
    COMPULSION("Compulsione", SpellSchool.ENCHANTMENT),
    CREATION("Creazione", SpellSchool.CONJURATION),
    FIGMENT("Finzione", SpellSchool.ILLUSION),
    GLAMER("Inganno", SpellSchool.ILLUSION),
    HEALING("Guarigione", SpellSchool.CONJURATION),
    PATTERN("Trama", SpellSchool.ILLUSION),
    PHANTASM("Allucinazione", SpellSchool.ILLUSION),
    POLYMORPH("Metamorfosi", SpellSchool.TRANSMUTATION),
    SCRYING("Scrutamento", SpellSchool.DIVINATION),
    SHADOW("Ombra", SpellSchool.ILLUSION),
    SUMMONING("Convocazione", SpellSchool.CONJURATION),
    TELEPORTATION("Teletrasporto", SpellSchool.CONJURATION);

    private final String displayName;
    private final SpellSchool spellSchool;

    SpellSubschool(String displayName, SpellSchool spellSchool)
    {
        this.displayName = displayName;
        this.spellSchool = spellSchool;
    }
}
