package org.golarion.model;

import lombok.Getter;
import lombok.Setter;
import org.golarion.model.stats.Stats;

@Setter
@Getter
public class Sheet
{

    private final Stats stats;

    public Sheet()
    {
        stats = new Stats();
    }
}
