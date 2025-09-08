package org.golarion.model;

import lombok.Getter;
import lombok.Setter;
import org.golarion.model.stats.Stats;
import org.golarion.model.spell.SpellList;

@Setter
@Getter
public class Sheet
{

    private final Stats stats;
    private final SpellList spellList;

    public Sheet()
    {
        stats = new Stats();
        spellList = new SpellList();

        spellList.modifyMaxCounter(0, 3);
        spellList.modifyMaxCounter(1, 2);
        spellList.modifyMaxCounter(2, 1);
    }
}
