package org.golarion.model.spell;

import java.util.ArrayList;
import java.util.List;

import org.golarion.exceptions.MaxSpellSlotsException;

public class SpellList
{
    private final int[] maxSpellSlotsCounter;
    private final int[] currentSpellSlotsCounter;
    private final List<Spell>[] spellsByLevel;
    
    @SuppressWarnings("unchecked")
    public SpellList()
    {
        maxSpellSlotsCounter = new int[10];
        currentSpellSlotsCounter = new int[10];
        spellsByLevel = new List[10];
        for (int i = 0; i < spellsByLevel.length; i++)
        {
            spellsByLevel[i] = new ArrayList<>();
        }
    }

    public void modifyMaxCounter(int level, int delta)
    {
        if (level < 0 || level >= maxSpellSlotsCounter.length)
            throw new IllegalArgumentException("Level must be between 0 and " + (maxSpellSlotsCounter.length - 1));
        maxSpellSlotsCounter[level] += delta;
        if (maxSpellSlotsCounter[level] < 0)
            maxSpellSlotsCounter[level] = 0;
    }

    public void modifyCurrentCounter(int level, int delta)
    {
        if (level < 0 || level >= currentSpellSlotsCounter.length)
            throw new IllegalArgumentException("Level must be between 0 and " + (currentSpellSlotsCounter.length - 1));
        if (currentSpellSlotsCounter[level] + delta > maxSpellSlotsCounter[level])
            throw new MaxSpellSlotsException("You are out of spell slots for level " + level);
        currentSpellSlotsCounter[level] += delta;
        if (currentSpellSlotsCounter[level] < 0)
            currentSpellSlotsCounter[level] = 0;
    }

    public void addSpell(int level, Spell spell)
    {
        if (level < 0 || level >= spellsByLevel.length)
            throw new IllegalArgumentException("Level must be between 0 and " + (spellsByLevel.length - 1));
        spellsByLevel[level].add(spell);
    }
   
    public List<Spell> getSpellsByLevel(int level)
    {
        if (level < 0 || level >= spellsByLevel.length)
            throw new IllegalArgumentException("Level must be between 0 and " + (spellsByLevel.length - 1));
        return List.copyOf(spellsByLevel[level]);
    }

    public int getMaxSpellSlots(int level)
    {
        if (level < 0 || level >= maxSpellSlotsCounter.length)
            throw new IllegalArgumentException("Level must be between 0 and " + (maxSpellSlotsCounter.length - 1));
        return maxSpellSlotsCounter[level];
    }
}


