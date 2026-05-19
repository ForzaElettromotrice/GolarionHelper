package org.golarion.model.spell;

import java.util.Set;

public interface DomainSpellListType
{
    String getDisplayName();

    Set<DomainType> getDomains();
}
