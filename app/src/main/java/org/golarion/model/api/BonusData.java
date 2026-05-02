package org.golarion.model.api;

import org.golarion.model.character.modifier.BonusType;

import java.util.UUID;

public record BonusData(
    UUID id,
    String source,
    BonusType bonusType,
    int value,
    boolean enabled,
    String description
) {}
