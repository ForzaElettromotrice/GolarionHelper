package org.golarion.model.character.attack;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.AttackEntryData;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.modifier.Modifier;
import org.golarion.model.character.modifier.TargetManager;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class AttackEntry
{
    @Getter
    private final UUID id;
    private final List<AttackDamage> damages;
    private final List<AttackDamage> criticalDamages;
    private final AttackModifierBucket attackModifiers;
    private final AttackModifierBucket damageModifiers;
    private final AttackModifierBucket criticalConfirmationModifiers;
    private final AttackModifierBucket criticalDamageModifiers;
    @NonNull
    private final String name;
    @Getter(lombok.AccessLevel.PACKAGE)
    @NonNull
    private AttackType attackType;
    @Setter
    @NonNull
    private AbilityType attackAbilityType;
    @Setter
    private AbilityType damageAbilityType;
    private int criticalThreatRange;
    private int criticalMultiplier;

    public AttackEntry(@NonNull String name)
    {
        this.id = UUID.randomUUID();
        this.damages = new ArrayList<>();
        this.criticalDamages = new ArrayList<>();
        this.name = validateName(name);
        this.attackType = AttackType.MELEE;
        this.attackAbilityType = AbilityType.STRENGTH;
        this.damageAbilityType = AbilityType.STRENGTH;
        this.criticalThreatRange = 20;
        this.criticalMultiplier = 2;
        this.attackModifiers = new AttackModifierBucket();
        this.damageModifiers = new AttackModifierBucket();
        this.criticalConfirmationModifiers = new AttackModifierBucket();
        this.criticalDamageModifiers = new AttackModifierBucket();
    }

    public AttackEntryData toData(int baseAttackBonus, @NonNull Map<AbilityType, Integer> abilityModifiers)
    {
        int attackAbilityModifier = getAbilityModifier(abilityModifiers, attackAbilityType);
        int damageAbilityModifier = damageAbilityType == null ? 0 : getAbilityModifier(abilityModifiers, damageAbilityType);
        int totalAttackBonus = baseAttackBonus + attackAbilityModifier + attackModifiers.getTotalValue();
        int totalCriticalDamageBonus = criticalDamageModifiers.getTotalValue();

        return new AttackEntryData(
                id,
                name,
                attackType,
                attackAbilityType,
                damageAbilityType,
                damages.stream().map(AttackDamage::toData).toList(),
                criticalDamages.stream().map(AttackDamage::toData).toList(),
                getTotalCriticalDamage(totalCriticalDamageBonus),
                criticalThreatRange,
                criticalMultiplier,
                attackModifiers.toData(),
                damageModifiers.toData(),
                criticalConfirmationModifiers.toData(),
                criticalDamageModifiers.toData(),
                totalAttackBonus,
                damageAbilityModifier + damageModifiers.getTotalValue(),
                totalAttackBonus + criticalConfirmationModifiers.getTotalValue(),
                totalCriticalDamageBonus
        );
    }

    public void registerModifierTargets(@NonNull TargetManager targetManager)
    {
        targetManager.registerModifierTarget(getAttackTargetName(name), attackModifiers);
        targetManager.registerModifierTarget(getDamageTargetName(name), damageModifiers);
        targetManager.registerModifierTarget(getCriticalConfirmationTargetName(name), criticalConfirmationModifiers);
        targetManager.registerModifierTarget(getCriticalDamageTargetName(name), criticalDamageModifiers);
    }

    public void unregisterModifierTargets(@NonNull TargetManager targetManager)
    {
        targetManager.removeModifierTarget(getAttackTargetName(name));
        targetManager.removeModifierTarget(getDamageTargetName(name));
        targetManager.removeModifierTarget(getCriticalConfirmationTargetName(name));
        targetManager.removeModifierTarget(getCriticalDamageTargetName(name));
    }

    void addAttackModifier(@NonNull Modifier modifier)
    {
        attackModifiers.addModifier(modifier);
    }

    void removeAttackModifier(@NonNull UUID modifierId)
    {
        attackModifiers.removeModifier(modifierId);
    }

    void addDamageModifier(@NonNull Modifier modifier)
    {
        damageModifiers.addModifier(modifier);
    }

    void removeDamageModifier(@NonNull UUID modifierId)
    {
        damageModifiers.removeModifier(modifierId);
    }

    void addCriticalConfirmationModifier(@NonNull Modifier modifier)
    {
        criticalConfirmationModifiers.addModifier(modifier);
    }

    void removeCriticalConfirmationModifier(@NonNull UUID modifierId)
    {
        criticalConfirmationModifiers.removeModifier(modifierId);
    }

    void addCriticalDamageModifier(@NonNull Modifier modifier)
    {
        criticalDamageModifiers.addModifier(modifier);
    }

    void removeCriticalDamageModifier(@NonNull UUID modifierId)
    {
        criticalDamageModifiers.removeModifier(modifierId);
    }

    void addDamage(@NonNull String damage, @NonNull DamageType damageType)
    {
        damages.add(new AttackDamage(damage, damageType));
    }

    void addCriticalDamage(@NonNull String damage, @NonNull DamageType damageType)
    {
        criticalDamages.add(new AttackDamage(damage, damageType));
    }

    public void setCriticalThreatRange(int criticalThreatRange)
    {
        if (criticalThreatRange < 1 || criticalThreatRange > 20)
        {
            throw new IllegalArgumentException("criticalThreatRange must be between 1 and 20");
        }

        this.criticalThreatRange = criticalThreatRange;
    }

    public void setCriticalMultiplier(int criticalMultiplier)
    {
        if (criticalMultiplier < 2)
        {
            throw new IllegalArgumentException("criticalMultiplier must be at least 2");
        }

        this.criticalMultiplier = criticalMultiplier;
    }

    void removeDamage(@NonNull UUID damageId)
    {
        if (!damages.removeIf(damage -> damage.hasId(damageId)))
        {
            throw new IllegalArgumentException("damageId not found: " + damageId);
        }
    }

    void removeCriticalDamage(@NonNull UUID damageId)
    {
        if (!criticalDamages.removeIf(damage -> damage.hasId(damageId)))
        {
            throw new IllegalArgumentException("criticalDamageId not found: " + damageId);
        }
    }

    boolean isAttackType(@NonNull AttackType attackType)
    {
        return this.attackType == attackType;
    }

    void setAttackType(@NonNull AttackType attackType)
    {
        this.attackType = attackType;
    }

    boolean hasId(@NonNull UUID id)
    {
        return this.id.equals(id);
    }

    private String getAttackTargetName(@NonNull String name)
    {
        return name + "Attack";
    }

    private String getDamageTargetName(@NonNull String name)
    {
        return name + "Damage";
    }

    private String getCriticalConfirmationTargetName(@NonNull String name)
    {
        return name + "CriticalConfirmation";
    }

    private String getCriticalDamageTargetName(@NonNull String name)
    {
        return name + "CriticalDamage";
    }

    private int getAbilityModifier(@NonNull Map<AbilityType, Integer> abilityModifiers, @NonNull AbilityType abilityType)
    {
        Integer abilityModifier = abilityModifiers.get(abilityType);
        if (abilityModifier == null)
        {
            throw new IllegalArgumentException("ability modifier not found: " + abilityType);
        }

        return abilityModifier;
    }

    private String getTotalCriticalDamage(int criticalDamageBonus)
    {
        List<String> criticalDamageParts = new ArrayList<>();

        for (AttackDamage damage : damages)
        {
            criticalDamageParts.add(damage.getCriticalDamage(criticalMultiplier).toString());
        }
        for (AttackDamage damage : criticalDamages)
        {
            criticalDamageParts.add(damage.getDamage().toString());
        }

        if (criticalDamageBonus != 0)
        {
            addSignedNumber(criticalDamageParts, criticalDamageBonus);
        }

        return String.join(" + ", criticalDamageParts);
    }

    private void addSignedNumber(@NonNull List<String> parts, int value)
    {
        if (parts.isEmpty() || value > 0)
        {
            parts.add(String.valueOf(value));
            return;
        }

        parts.add("- " + Math.abs(value));
    }

    private String validateName(@NonNull String name)
    {
        String normalizedName = name.trim();
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("name must not be blank");
        }

        return normalizedName;
    }
}
