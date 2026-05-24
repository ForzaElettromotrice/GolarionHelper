package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;

public enum NoAction implements Action, ReverseAction
{
    INSTANCE;

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        return this;
    }

    @Override
    public void apply()
    {
    }
}
