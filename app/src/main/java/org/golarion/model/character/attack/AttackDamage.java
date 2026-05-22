package org.golarion.model.character.attack;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.AttackDamageData;

import java.util.UUID;

public class AttackDamage
{
    @Getter
    private final UUID id;
    @NonNull
    private final DamageType damageType;
    @Getter(lombok.AccessLevel.PACKAGE)
    private final DamageDice damage;

    public AttackDamage(@NonNull String damage, @NonNull DamageType damageType)
    {
        this.id = UUID.randomUUID();
        this.damage = DamageDice.parse(damage);
        this.damageType = damageType;
    }

    AttackDamageData toData()
    {
        return new AttackDamageData(id, damage.toString(), damageType);
    }

    DamageDice getCriticalDamage(int criticalMultiplier)
    {
        return damage.multiply(criticalMultiplier);
    }

    boolean hasId(@NonNull UUID id)
    {
        return this.id.equals(id);
    }

}
