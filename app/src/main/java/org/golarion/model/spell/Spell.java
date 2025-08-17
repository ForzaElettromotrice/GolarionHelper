package org.golarion.model.spell;

import java.util.List;
import java.util.Map;

public record Spell(String name, School school, Descriptor descriptor, Map<GClass, Integer> classLevels,
                    List<Domain> domain, Bloodline bloodline, List<Components> components, List<CastTime> castTime,
                    String castTimeDescription, List<GRange> range, String rangeDescription,
                    String target, String effect, String area, List<GDuration> duration, String durationDescription,
                    List<SavingThrow> savingThrow, String savingThrowDescription,
                    List<String> spellResistance, String spellResistanceDescription, String description)
{

}
