package org.golarion.model.api;

import org.golarion.model.spell.SpellFilterMatchMode;

import java.util.Collection;
import java.util.Set;

public record SpellFilterCriterion<T>(
        Set<T> values,
        SpellFilterMatchMode matchMode
)
{
    public SpellFilterCriterion
    {
        values = values == null ? Set.of() : Set.copyOf(values);
        matchMode = matchMode == null ? SpellFilterMatchMode.ANY : matchMode;
    }

    public boolean isActive()
    {
        return !values.isEmpty();
    }

    public boolean matches(Collection<T> candidateValues)
    {
        if (!isActive())
        {
            return true;
        }
        if (candidateValues == null || candidateValues.isEmpty())
        {
            return false;
        }

        return matchMode == SpellFilterMatchMode.ALL
                ? candidateValues.containsAll(values)
                : values.stream().anyMatch(candidateValues::contains);
    }
}
