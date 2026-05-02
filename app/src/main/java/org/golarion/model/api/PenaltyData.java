package org.golarion.model.api;

import java.util.UUID;

public record PenaltyData(
    UUID id,
    String source,
    int value,
    boolean enabled,
    String description
) {}
