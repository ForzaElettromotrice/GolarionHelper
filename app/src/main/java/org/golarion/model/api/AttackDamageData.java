package org.golarion.model.api;

import org.golarion.model.character.attack.DamageType;

import java.util.UUID;

public record AttackDamageData(
        UUID id,
        String damage,
        DamageType damageType
)
{
}
