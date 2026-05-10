package org.golarion.model.character.modifier;

import lombok.NonNull;

import java.util.UUID;

public interface ModifierTarget
{
    void addModifier(@NonNull Modifier modifier);

    void removeModifier(@NonNull UUID modifierId);
}
