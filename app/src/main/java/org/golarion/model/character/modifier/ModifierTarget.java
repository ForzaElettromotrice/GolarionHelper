package org.golarion.model.character.modifier;

import lombok.NonNull;

import java.util.UUID;

public interface ModifierTarget
{
    void addModifier(@NonNull Modifier modifier);

    void removeModifier(@NonNull UUID modifierId);

    default void addConditionalModifier(@NonNull ConditionalModifier modifier)
    {
        throw new UnsupportedOperationException("conditional modifiers are not supported by this target yet");
    }

    default void removeConditionalModifier(@NonNull UUID modifierId)
    {
        throw new UnsupportedOperationException("conditional modifiers are not supported by this target yet");
    }
}
