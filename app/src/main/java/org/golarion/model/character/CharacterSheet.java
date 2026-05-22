package org.golarion.model.character;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.*;
import org.golarion.model.character.ability.AbilityScore;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.armorclass.ArmorClassEntry;
import org.golarion.model.character.attack.AttackEntry;
import org.golarion.model.character.attack.AttackStats;
import org.golarion.model.character.attack.AttackType;
import org.golarion.model.character.attack.DamageType;
import org.golarion.model.character.hitpoints.HitPointField;
import org.golarion.model.character.hitpoints.HitPointsEntry;
import org.golarion.model.character.initiative.InitiativeEntry;
import org.golarion.model.character.modifier.*;
import org.golarion.model.character.savingthrow.SavingThrowEntry;
import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;

import java.util.*;

public class CharacterSheet
{
    private final EnumMap<AbilityType, AbilityScore> abilityScores;
    private final EnumMap<SavingThrowType, SavingThrowEntry> savingThrows;
    private final ArmorClassEntry armorClass;
    private final HitPointsEntry hitPoints;
    private final InitiativeEntry initiative;
    private final Skills skills;
    private final AttackStats attackStats;
    private final TargetManager targetManager;
    private final Map<UUID, EffectGroup> effectGroups;
    @Getter
    private String characterName;

    public CharacterSheet()
    {
        this.characterName = "Franco";
        this.abilityScores = new EnumMap<>(AbilityType.class);
        for (AbilityType abilityType : AbilityType.values())
        {
            abilityScores.put(abilityType, new AbilityScore(10));
        }
        this.savingThrows = new EnumMap<>(SavingThrowType.class);
        for (SavingThrowType savingThrowType : SavingThrowType.values())
        {
            savingThrows.put(savingThrowType, new SavingThrowEntry());
        }
        this.armorClass = new ArmorClassEntry();
        this.hitPoints = new HitPointsEntry();
        this.hitPoints.setMaxHpModifierResolver(this::getHitPointsConstitutionModifier);
        this.initiative = new InitiativeEntry();
        this.skills = new Skills();
        this.attackStats = new AttackStats();
        setCharacterName(characterName);

        this.targetManager = new TargetManager();
        this.effectGroups = new LinkedHashMap<>();

        registerModifierTargets();
        registerDeltaTargets();
        registerValueTargets();
        registerDerivedTargetVariables();
    }

    public void setCharacterName(@NonNull String characterName)
    {
        String normalizedName = characterName.trim();
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("characterName must not be blank");
        }

        this.characterName = normalizedName;
    }


    public UUID createEffectGroup(@NonNull String name)
    {
        EffectGroup group = new EffectGroup(name);
        effectGroups.put(group.getId(), group);
        return group.getId();
    }

    public UUID addEffect(@NonNull UUID effectGroup, @NonNull ModifierType modifierType, BonusType bonusType, @NonNull String expression, @NonNull String targetString, @NonNull String source, @NonNull String description)
    {
        Expression exp = new Expression(targetManager, expression);
        targetManager.validateTargetExpression(targetString, exp);
        EffectGroup group = getEffectGroup(effectGroup);

        boolean isModifierTarget = targetManager.hasModifierTarget(targetString);
        boolean isDeltaTarget = targetManager.hasDeltaTarget(targetString);

        if (isModifierTarget == isDeltaTarget)
        {
            throw new IllegalArgumentException("target must resolve to exactly one operational target: " + targetString);
        }

        if (isModifierTarget)
        {
            Modifier mod = new Modifier(modifierType, source, true, description, bonusType, exp);
            ModifierTarget target = targetManager.getModifierTarget(targetString);
            target.addModifier(mod);
            try
            {
                targetManager.resolveValue(targetString);
            }
            catch (IllegalArgumentException exception)
            {
                target.removeModifier(mod.getId());
                throw exception;
            }
            return group.addEntry(new ModifierEffectEntry(targetString, target, mod));
        }

        DeltaTarget target = targetManager.getDeltaTarget(targetString);
        return group.addEntry(new DeltaEffectEntry(targetString, target, source, description, exp));
    }

    public void removeEffect(@NonNull UUID effectGroup, @NonNull UUID effectId)
    {
        getEffectGroup(effectGroup).removeEffect(effectId);
    }

    public void setEffectGroupEnabled(@NonNull UUID effectGroup, boolean enabled)
    {
        getEffectGroup(effectGroup).setEnabled(enabled);
    }

    public void setEffectGroupName(@NonNull UUID effectGroup, @NonNull String name)
    {
        getEffectGroup(effectGroup).setName(name);
    }

    public void removeEffectGroup(@NonNull UUID effectGroup)
    {
        EffectGroup group = getEffectGroup(effectGroup);
        group.removeAllEffects();
        effectGroups.remove(effectGroup);
    }

    public List<EffectGroupData> getEffectGroups()
    {
        return effectGroups.values().stream().map(EffectGroup::toData).toList();
    }

    public List<String> getEffectTargets()
    {
        LinkedHashSet<String> targets = new LinkedHashSet<>(targetManager.getModifierTargetNames());
        targets.addAll(targetManager.getDeltaTargetNames());
        return targets.stream().toList();
    }

    public void setAbilityBaseValue(@NonNull AbilityType abilityType, int baseValue)
    {
        getAbilityScore(abilityType).setBaseValue(baseValue);
    }

    public AbilityData getAbility(AbilityType abilityType)
    {
        return getAbilityScore(abilityType).toData(abilityType);
    }

    public void setSavingThrowBaseValue(@NonNull SavingThrowType savingThrowType, int baseValue)
    {
        getSavingThrowEntry(savingThrowType).setBaseValue(baseValue);
    }

    public SavingThrowData getSavingThrow(@NonNull SavingThrowType savingThrowType)
    {
        int abilityModifier = getAbilityScore(savingThrowType.getKeyAbility()).getModifier();
        return getSavingThrowEntry(savingThrowType).toData(savingThrowType, abilityModifier);
    }

    public ArmorClassData getArmorClass()
    {
        int dexterityModifier = getAbilityScore(AbilityType.DEXTERITY).getModifier();
        return armorClass.toData(dexterityModifier);
    }

    public void setHitPoints(@NonNull HitPointField field, int value)
    {
        hitPoints.set(field, value);
    }

    public void changeHitPoints(@NonNull HitPointField field, int delta)
    {
        hitPoints.change(field, delta);
    }

    public HitPointsData getHitPoints()
    {
        return hitPoints.toData();
    }

    public void setInitiativeBaseValue(int baseValue)
    {
        initiative.setBaseValue(baseValue);
    }

    public InitiativeData getInitiative()
    {
        int dexterityModifier = getAbilityScore(AbilityType.DEXTERITY).getModifier();
        return initiative.toData(dexterityModifier);
    }

    public List<String> getSkillSpecializations(@NonNull SkillType skillType)
    {
        return skills.getSpecializations(skillType);
    }

    public void addSkillSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {

        skills.addSpecialization(skillType, specialization.trim());
    }

    public void removeSkillSpecialization(@NonNull SkillType skillType, @NonNull String specialization)
    {
        if (!skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + " does not support specialization");
        }
        skills.removeSpecialization(skillType, specialization);
    }

    public void setSkillRanks(@NonNull SkillType skillType, int ranks)
    {
        if (skillType.isRequiresSpecialization())
        {
            throw new IllegalArgumentException("skillType " + skillType + "cannot have ranks without specialization");
        }
        skills.get(skillType).setRanks(ranks);
    }

    public void setSkillRanks(@NonNull SkillType skillType, @NonNull String specialization, int ranks)
    {
        skills.getSpecialization(skillType, specialization).setRanks(ranks);
    }

    public void setSkillClassSkill(@NonNull SkillType skillType, boolean classSkill)
    {
        skills.setClassSkill(skillType, classSkill);
    }

    public void setSkillAbilityType(@NonNull SkillType skillType, @NonNull AbilityType abilityType)
    {
        skills.setAbilityType(skillType, abilityType);
    }

    public SkillData getSkill(@NonNull SkillType skillType)
    {
        return toSkillData(skillType, "", skills.get(skillType));
    }

    public SkillData getSkill(@NonNull SkillType skillType, @NonNull String specialization)
    {
        return toSkillData(skillType, specialization, skills.getSpecialization(skillType, specialization));
    }

    public UUID addAttack(@NonNull String name)
    {
        AttackEntry attack = new AttackEntry(name);
        attackStats.addAttack(attack, targetManager);
        return attack.getId();
    }

    public void removeAttack(@NonNull UUID attackId)
    {
        attackStats.removeAttack(attackId, targetManager);
    }

    public void setAttackType(@NonNull UUID attackId, @NonNull AttackType attackType)
    {
        attackStats.setAttackType(attackId, attackType);
    }

    public void setAttackAbilityType(@NonNull UUID attackId, @NonNull AbilityType abilityType)
    {
        attackStats.setAttackAbilityType(attackId, abilityType);
    }

    public void setAttackDamageAbilityType(@NonNull UUID attackId, AbilityType abilityType)
    {
        attackStats.setDamageAbilityType(attackId, abilityType);
    }

    public void addAttackDamage(@NonNull UUID attackId, @NonNull String damage, @NonNull DamageType damageType)
    {
        attackStats.addDamage(attackId, damage, damageType);
    }

    public void removeAttackDamage(@NonNull UUID attackId, @NonNull UUID damageId)
    {
        attackStats.removeDamage(attackId, damageId);
    }

    public void addAttackCriticalDamage(@NonNull UUID attackId, @NonNull String damage, @NonNull DamageType damageType)
    {
        attackStats.addCriticalDamage(attackId, damage, damageType);
    }

    public void removeAttackCriticalDamage(@NonNull UUID attackId, @NonNull UUID damageId)
    {
        attackStats.removeCriticalDamage(attackId, damageId);
    }

    public void setAttackCriticalThreatRange(@NonNull UUID attackId, int criticalThreatRange)
    {
        attackStats.setCriticalThreatRange(attackId, criticalThreatRange);
    }

    public void setAttackCriticalMultiplier(@NonNull UUID attackId, int criticalMultiplier)
    {
        attackStats.setCriticalMultiplier(attackId, criticalMultiplier);
    }

    public AttackStatsData getAttackStats()
    {
        return attackStats.toData(getBaseAttackBonus(), getAbilityModifiers());
    }

    private AbilityScore getAbilityScore(@NonNull AbilityType abilityType)
    {
        return abilityScores.get(abilityType);
    }

    private SavingThrowEntry getSavingThrowEntry(@NonNull SavingThrowType savingThrowType)
    {
        return savingThrows.get(savingThrowType);
    }

    private int getHitPointsConstitutionModifier()
    {
        return getAbilityScore(AbilityType.CONSTITUTION).getModifier() * getTotalLevel();
    }

    private int getTotalLevel()
    {
        return 1;
    }

    private int getBaseAttackBonus()
    {
        return 0;
    }

    private EffectGroup getEffectGroup(@NonNull UUID effectGroupId)
    {
        EffectGroup effectGroup = effectGroups.get(effectGroupId);
        if (effectGroup == null)
        {
            throw new IllegalArgumentException("effectGroup not found: " + effectGroupId);
        }

        return effectGroup;
    }

    private void registerModifierTargets()
    {
        targetManager.registerModifierTarget("strength", getAbilityScore(AbilityType.STRENGTH));
        targetManager.registerModifierTarget("dexterity", getAbilityScore(AbilityType.DEXTERITY));
        targetManager.registerModifierTarget("constitution", getAbilityScore(AbilityType.CONSTITUTION));
        targetManager.registerModifierTarget("intelligence", getAbilityScore(AbilityType.INTELLIGENCE));
        targetManager.registerModifierTarget("wisdom", getAbilityScore(AbilityType.WISDOM));
        targetManager.registerModifierTarget("charisma", getAbilityScore(AbilityType.CHARISMA));
        targetManager.registerModifierTarget("fortitude", getSavingThrowEntry(SavingThrowType.FORTITUDE));
        targetManager.registerModifierTarget("reflex", getSavingThrowEntry(SavingThrowType.REFLEX));
        targetManager.registerModifierTarget("will", getSavingThrowEntry(SavingThrowType.WILL));
        targetManager.registerModifierTarget("armorClass", armorClass);
        targetManager.registerModifierTarget("initiative", initiative);
        attackStats.registerModifierTargets(targetManager);
    }

    private void registerDeltaTargets()
    {
        targetManager.registerDeltaTarget("maxHp", delta -> hitPoints.change(HitPointField.MAX, delta));
        targetManager.registerDeltaTarget("currentHp", delta -> hitPoints.change(HitPointField.CURRENT, delta));
        targetManager.registerDeltaTarget("temporaryHp", delta -> hitPoints.change(HitPointField.TEMPORARY, delta));
        targetManager.registerDeltaTarget("nonlethalDamage", delta -> hitPoints.change(HitPointField.NONLETHAL, delta));
    }

    private void registerValueTargets()
    {
        targetManager.registerValueTarget("strength", () -> getAbility(AbilityType.STRENGTH).totalValue());
        targetManager.registerValueTarget("dexterity", () -> getAbility(AbilityType.DEXTERITY).totalValue());
        targetManager.registerValueTarget("constitution", () -> getAbility(AbilityType.CONSTITUTION).totalValue());
        targetManager.registerValueTarget("intelligence", () -> getAbility(AbilityType.INTELLIGENCE).totalValue());
        targetManager.registerValueTarget("wisdom", () -> getAbility(AbilityType.WISDOM).totalValue());
        targetManager.registerValueTarget("charisma", () -> getAbility(AbilityType.CHARISMA).totalValue());

        targetManager.registerValueTarget("strengthModifier", () -> getAbility(AbilityType.STRENGTH).modifier());
        targetManager.registerValueTarget("dexterityModifier", () -> getAbility(AbilityType.DEXTERITY).modifier());
        targetManager.registerValueTarget("constitutionModifier", () -> getAbility(AbilityType.CONSTITUTION).modifier());
        targetManager.registerValueTarget("intelligenceModifier", () -> getAbility(AbilityType.INTELLIGENCE).modifier());
        targetManager.registerValueTarget("wisdomModifier", () -> getAbility(AbilityType.WISDOM).modifier());
        targetManager.registerValueTarget("charismaModifier", () -> getAbility(AbilityType.CHARISMA).modifier());
    }

    private void registerDerivedTargetVariables()
    {
        targetManager.registerForbiddenVariables("strength", "strengthModifier");
        targetManager.registerForbiddenVariables("dexterity", "dexterityModifier");
        targetManager.registerForbiddenVariables("constitution", "constitutionModifier");
        targetManager.registerForbiddenVariables("intelligence", "intelligenceModifier");
        targetManager.registerForbiddenVariables("wisdom", "wisdomModifier");
        targetManager.registerForbiddenVariables("charisma", "charismaModifier");
    }

    private SkillData toSkillData(@NonNull SkillType skillType, @NonNull String specialization, @NonNull org.golarion.model.character.skill.SkillEntry skillEntry)
    {
        int abilityModifier = getAbilityScore(skillEntry.getAbilityType()).getModifier();
        return skillEntry.toData(skillType, specialization, abilityModifier);
    }

    private EnumMap<AbilityType, Integer> getAbilityModifiers()
    {
        EnumMap<AbilityType, Integer> abilityModifiers = new EnumMap<>(AbilityType.class);
        for (AbilityType abilityType : AbilityType.values())
        {
            abilityModifiers.put(abilityType, getAbilityScore(abilityType).getModifier());
        }

        return abilityModifiers;
    }
}
