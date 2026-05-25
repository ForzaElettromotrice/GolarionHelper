package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;

import java.util.ArrayList;
import java.util.List;

public class ActionGroup implements Action
{
    private final List<Action> actions;

    public ActionGroup(@NonNull List<Action> actions)
    {
        this.actions = List.copyOf(actions);
    }

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        List<ReverseAction> reverseActions = new ArrayList<>();
        try
        {
            for (Action action : actions)
            {
                reverseActions.add(action.apply(characterSheet));
            }
        } catch (RuntimeException exception)
        {
            reverseActions.reversed().forEach(ReverseAction::apply);
            throw exception;
        }

        return () -> reverseActions.reversed().forEach(ReverseAction::apply);
    }
}
