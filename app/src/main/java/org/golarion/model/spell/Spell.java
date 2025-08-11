package org.golarion.model.spell;

import java.util.Map;

public record Spell(String name, School school, Descriptor descriptor, Map<GClass, Integer> classLevels, Domain domain,
                    Bloodline bloodline, String components, String castTime, String range, String target,
                    String effect, String area, String duration, String savingThrow, String spellResistance,
                    String description)
{

}
