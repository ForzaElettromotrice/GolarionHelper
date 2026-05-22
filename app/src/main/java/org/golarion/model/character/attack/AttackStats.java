package org.golarion.model.character.attack;

import lombok.NonNull;
import org.golarion.model.api.AttackEntryData;
import org.golarion.model.api.AttackStatsData;
import org.golarion.model.api.AttackTypeModifierData;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.ModifierTarget;
import org.golarion.model.character.modifier.TargetManager;

import java.util.*;

public class AttackStats
{
    private enum GlobalAttackModifierTargetType
    {
        ATTACK,
        DAMAGE,
        CRITICAL_ATTACK,
        CRITICAL_DAMAGE
    }

    private class GlobalAttackModifierTarget implements ModifierTarget
    {
        private final GlobalAttackModifierTargetType targetType;
        private final List<Modifier> modifiers;

        private GlobalAttackModifierTarget(@NonNull GlobalAttackModifierTargetType targetType)
        {
            this.targetType = targetType;
            this.modifiers = new ArrayList<>();
        }

        @Override
        public void addModifier(@NonNull Modifier modifier)
        {
            modifiers.add(modifier);
            for (AttackEntry attack : attacks)
            {
                applyModifierTo(attack, modifier);
            }
        }

        @Override
        public void removeModifier(@NonNull UUID modifierId)
        {
            modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
            for (AttackEntry attack : attacks)
            {
                removeModifierFrom(attack, modifierId);
            }
        }

        private void applyStoredModifiersTo(@NonNull AttackEntry attack)
        {
            for (Modifier modifier : modifiers)
            {
                applyModifierTo(attack, modifier);
            }
        }

        private void removeStoredModifiersFrom(@NonNull AttackEntry attack)
        {
            for (Modifier modifier : modifiers)
            {
                removeModifierFrom(attack, modifier.getId());
            }
        }

        private void applyModifierTo(@NonNull AttackEntry attack, @NonNull Modifier modifier)
        {
            switch (targetType)
            {
                case ATTACK -> attack.addAttackModifier(modifier);
                case DAMAGE -> attack.addDamageModifier(modifier);
                case CRITICAL_ATTACK -> attack.addCriticalConfirmationModifier(modifier);
                case CRITICAL_DAMAGE -> attack.addCriticalDamageModifier(modifier);
            }
        }

        private void removeModifierFrom(@NonNull AttackEntry attack, @NonNull UUID modifierId)
        {
            switch (targetType)
            {
                case ATTACK -> attack.removeAttackModifier(modifierId);
                case DAMAGE -> attack.removeDamageModifier(modifierId);
                case CRITICAL_ATTACK -> attack.removeCriticalConfirmationModifier(modifierId);
                case CRITICAL_DAMAGE -> attack.removeCriticalDamageModifier(modifierId);
            }
        }
    }

    private class AttackTypeModifierTarget implements ModifierTarget
    {
        private final AttackType attackType;
        private final List<Modifier> modifiers;

        private AttackTypeModifierTarget(@NonNull AttackType attackType)
        {
            this.attackType = attackType;
            this.modifiers = new ArrayList<>();
        }

        @Override
        public void addModifier(@NonNull Modifier modifier)
        {
            modifiers.add(modifier);
            for (AttackEntry attack : attacks)
            {
                if (attack.isAttackType(attackType))
                {
                    attack.addAttackModifier(modifier);
                }
            }
        }

        @Override
        public void removeModifier(@NonNull UUID modifierId)
        {
            modifiers.removeIf(modifier -> modifier.getId().equals(modifierId));
            for (AttackEntry attack : attacks)
            {
                if (attack.isAttackType(attackType))
                {
                    attack.removeAttackModifier(modifierId);
                }
            }
        }

        private void applyStoredModifiersTo(@NonNull AttackEntry attack)
        {
            if (!attack.isAttackType(attackType))
            {
                return;
            }

            for (Modifier modifier : modifiers)
            {
                attack.addAttackModifier(modifier);
            }
        }

        private void removeStoredModifiersFrom(@NonNull AttackEntry attack)
        {
            if (!attack.isAttackType(attackType))
            {
                return;
            }

            for (Modifier modifier : modifiers)
            {
                attack.removeAttackModifier(modifier.getId());
            }
        }

        private AttackTypeModifierData toData()
        {
            return new AttackTypeModifierData(
                    attackType,
                    Modifier.calculateTotal(modifiers),
                    modifiers.stream().map(Modifier::toData).toList()
            );
        }
    }

    private final List<AttackEntry> attacks;
    private final GlobalAttackModifierTarget attackTarget;
    private final GlobalAttackModifierTarget damageTarget;
    private final GlobalAttackModifierTarget criticalAttackTarget;
    private final GlobalAttackModifierTarget criticalDamageTarget;
    private final Map<AttackType, AttackTypeModifierTarget> attackTypeTargets;

    public AttackStats()
    {
        this.attacks = new ArrayList<>();
        this.attackTarget = new GlobalAttackModifierTarget(GlobalAttackModifierTargetType.ATTACK);
        this.damageTarget = new GlobalAttackModifierTarget(GlobalAttackModifierTargetType.DAMAGE);
        this.criticalAttackTarget = new GlobalAttackModifierTarget(GlobalAttackModifierTargetType.CRITICAL_ATTACK);
        this.criticalDamageTarget = new GlobalAttackModifierTarget(GlobalAttackModifierTargetType.CRITICAL_DAMAGE);
        this.attackTypeTargets = new EnumMap<>(AttackType.class);
        for (AttackType attackType : AttackType.values())
        {
            attackTypeTargets.put(attackType, new AttackTypeModifierTarget(attackType));
        }
    }

    public void addAttack(@NonNull AttackEntry attack, @NonNull TargetManager targetManager)
    {
        attack.registerModifierTargets(targetManager);
        attacks.add(attack);
        attackTarget.applyStoredModifiersTo(attack);
        damageTarget.applyStoredModifiersTo(attack);
        criticalAttackTarget.applyStoredModifiersTo(attack);
        criticalDamageTarget.applyStoredModifiersTo(attack);
        attackTypeTargets.get(attack.getAttackType()).applyStoredModifiersTo(attack);
    }

    public void removeAttack(@NonNull UUID attackId, @NonNull TargetManager targetManager)
    {
        AttackEntry attack = getAttack(attackId);
        attack.unregisterModifierTargets(targetManager);
        attackTypeTargets.get(attack.getAttackType()).removeStoredModifiersFrom(attack);
        attackTarget.removeStoredModifiersFrom(attack);
        damageTarget.removeStoredModifiersFrom(attack);
        criticalAttackTarget.removeStoredModifiersFrom(attack);
        criticalDamageTarget.removeStoredModifiersFrom(attack);
        attacks.remove(attack);
    }

    public void setAttackType(@NonNull UUID attackId, @NonNull AttackType attackType)
    {
        AttackEntry attack = getAttack(attackId);
        if (attack.isAttackType(attackType))
            return;

        attackTypeTargets.get(attack.getAttackType()).removeStoredModifiersFrom(attack);
        attack.setAttackType(attackType);
        attackTypeTargets.get(attackType).applyStoredModifiersTo(attack);

    }

    public void setAttackAbilityType(@NonNull UUID attackId, @NonNull AbilityType abilityType)
    {
        getAttack(attackId).setAttackAbilityType(abilityType);
    }

    public void setDamageAbilityType(@NonNull UUID attackId, AbilityType abilityType)
    {
        getAttack(attackId).setDamageAbilityType(abilityType);
    }

    public void addDamage(@NonNull UUID attackId, @NonNull String damage, @NonNull DamageType damageType)
    {
        getAttack(attackId).addDamage(damage, damageType);
    }

    public void removeDamage(@NonNull UUID attackId, @NonNull UUID damageId)
    {
        getAttack(attackId).removeDamage(damageId);
    }

    public void addCriticalDamage(@NonNull UUID attackId, @NonNull String damage, @NonNull DamageType damageType)
    {
        getAttack(attackId).addCriticalDamage(damage, damageType);
    }

    public void removeCriticalDamage(@NonNull UUID attackId, @NonNull UUID damageId)
    {
        getAttack(attackId).removeCriticalDamage(damageId);
    }

    public void setCriticalThreatRange(@NonNull UUID attackId, int criticalThreatRange)
    {
        getAttack(attackId).setCriticalThreatRange(criticalThreatRange);
    }

    public void setCriticalMultiplier(@NonNull UUID attackId, int criticalMultiplier)
    {
        getAttack(attackId).setCriticalMultiplier(criticalMultiplier);
    }

    public void registerModifierTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget("Attack", attackTarget);
        targetManager.registerModifierTarget("Damage", damageTarget);
        targetManager.registerModifierTarget("CriticalAttack", criticalAttackTarget);
        targetManager.registerModifierTarget("CriticalDamage", criticalDamageTarget);
        for (Map.Entry<AttackType, AttackTypeModifierTarget> entry : attackTypeTargets.entrySet())
        {
            targetManager.registerModifierTarget(entry.getKey().toString(), entry.getValue());
        }
    }

    public AttackStatsData toData(int baseAttackBonus, @NonNull Map<AbilityType, Integer> abilityModifiers)
    {
        return new AttackStatsData(
                attacks.stream()
                        .map(attack -> attack.toData(baseAttackBonus, abilityModifiers))
                        .toList(),
                attackTypeTargets.values().stream()
                        .map(AttackTypeModifierTarget::toData)
                        .toList()
        );
    }

    private AttackEntry getAttack(@NonNull UUID attackId)
    {
        return attacks.stream()
                .filter(attack -> attack.hasId(attackId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("attackId not found: " + attackId));
    }
}
