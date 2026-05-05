package org.golarion.model.api;

public record HitPointsData(
        int maxHp,
        int currentHp,
        int temporaryHp,
        int nonlethalDamage
)
{
}
