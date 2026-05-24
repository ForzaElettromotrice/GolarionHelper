package org.golarion.model.character.action;

import lombok.NonNull;

import java.util.UUID;

public record ActionSource(
        @NonNull ActionSourceType type,
        @NonNull UUID id
)
{
}
