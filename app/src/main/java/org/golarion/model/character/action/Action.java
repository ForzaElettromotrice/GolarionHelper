package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;

public interface Action
{
    @NonNull
    ReverseAction apply(@NonNull CharacterSheet characterSheet);
}
