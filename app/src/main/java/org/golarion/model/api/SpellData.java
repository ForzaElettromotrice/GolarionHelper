package org.golarion.model.api;

import org.golarion.model.character.characterclass.CharacterClassType;
import org.golarion.model.spell.*;

import java.util.EnumSet;
import java.util.List;
import java.util.Map;

public record SpellData(
        String name,
        SpellSchool school,
        SpellSubschool subschool,
        EnumSet<SpellDescriptor> descriptors,
        Map<CharacterClassType, Integer> spellLevelByClass,
        Map<DomainSpellListType, Integer> spellLevelByDomain,
        Map<BloodlineType, Integer> spellLevelByBloodline,
        SpellCastingTimeCategory castingTimeCategory,
        String castingTimeDescription,
        EnumSet<SpellComponentType> components,
        String materialComponentDescription,
        Integer materialComponentCostInGoldPieces,
        String focusComponentDescription,
        SpellRangeType rangeType,
        String rangeDescription,
        String target,
        String effect,
        String area,
        SpellDurationType durationType,
        String durationDescription,
        boolean dismissible,
        List<SpellSavingThrowData> savingThrows,
        SpellResistanceType spellResistanceType,
        String spellResistanceDescription,
        String description
)
{
}
