package org.golarion.model.character;

import lombok.Getter;
import lombok.NonNull;
import org.golarion.model.api.*;
import org.golarion.model.character.ability.AbilityScore;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.armorclass.ArmorClassEntry;
import org.golarion.model.character.hitpoints.HitPointField;
import org.golarion.model.character.hitpoints.HitPointsEntry;
import org.golarion.model.character.initiative.InitiativeEntry;
import org.golarion.model.character.modifier.*;
import org.golarion.model.character.savingthrow.SavingThrowEntry;
import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;

import java.util.*;
import java.util.function.IntSupplier;

public class CharacterSheet
{
    private final EnumMap<AbilityType, AbilityScore> abilityScores;
    private final EnumMap<SavingThrowType, SavingThrowEntry> savingThrows;
    private final ArmorClassEntry armorClass;
    private final HitPointsEntry hitPoints;
    private final InitiativeEntry initiative;
    private final Skills skills;
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
        this.initiative = new InitiativeEntry();
        this.skills = new Skills();
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

    public SkillData getSkill(@NonNull SkillType skillType)
    {
        int abilityModifier = getAbilityScore(skillType.getKeyAbility()).getModifier();
        return skills.get(skillType).toData(skillType, "", abilityModifier);
    }

    public SkillData getSkill(@NonNull SkillType skillType, @NonNull String specialization)
    {
        int abilityModifier = getAbilityScore(skillType.getKeyAbility()).getModifier();
        return skills.getSpecialization(skillType, specialization).toData(skillType, specialization, abilityModifier);
    }

    private AbilityScore getAbilityScore(@NonNull AbilityType abilityType)
    {
        return abilityScores.get(abilityType);
    }

    private SavingThrowEntry getSavingThrowEntry(@NonNull SavingThrowType savingThrowType)
    {
        return savingThrows.get(savingThrowType);
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
        registerAbilityModifierTarget("strength", AbilityType.STRENGTH);
        registerAbilityModifierTarget("dexterity", AbilityType.DEXTERITY);
        registerAbilityModifierTarget("constitution", AbilityType.CONSTITUTION);
        registerAbilityModifierTarget("intelligence", AbilityType.INTELLIGENCE);
        registerAbilityModifierTarget("wisdom", AbilityType.WISDOM);
        registerAbilityModifierTarget("charisma", AbilityType.CHARISMA);

        registerSavingThrowModifierTarget("fortitude", SavingThrowType.FORTITUDE);
        registerSavingThrowModifierTarget("reflex", SavingThrowType.REFLEX);
        registerSavingThrowModifierTarget("will", SavingThrowType.WILL);

        targetManager.registerModifierTarget("armorClass", new ModifierTarget()
        {
            @Override
            public void addModifier(@NonNull Modifier modifier)
            {
                armorClass.addModifier(modifier);
            }

            @Override
            public void removeModifier(@NonNull UUID modifierId)
            {
                armorClass.removeModifier(modifierId);
            }
        });

        targetManager.registerModifierTarget("initiative", new ModifierTarget()
        {
            @Override
            public void addModifier(@NonNull Modifier modifier)
            {
                initiative.addModifier(modifier);
            }

            @Override
            public void removeModifier(@NonNull UUID modifierId)
            {
                initiative.removeModifier(modifierId);
            }
        });
    }

    private void registerDeltaTargets()
    {
        registerHitPointsDeltaTarget("maxHp", HitPointField.MAX);
        registerHitPointsDeltaTarget("currentHp", HitPointField.CURRENT);
        registerHitPointsDeltaTarget("temporaryHp", HitPointField.TEMPORARY);
        registerHitPointsDeltaTarget("nonlethalDamage", HitPointField.NONLETHAL);
    }

    private void registerValueTargets()
    {
        registerValueTarget("strength", () -> getAbility(AbilityType.STRENGTH).totalValue());
        registerValueTarget("dexterity", () -> getAbility(AbilityType.DEXTERITY).totalValue());
        registerValueTarget("constitution", () -> getAbility(AbilityType.CONSTITUTION).totalValue());
        registerValueTarget("intelligence", () -> getAbility(AbilityType.INTELLIGENCE).totalValue());
        registerValueTarget("wisdom", () -> getAbility(AbilityType.WISDOM).totalValue());
        registerValueTarget("charisma", () -> getAbility(AbilityType.CHARISMA).totalValue());

        registerValueTarget("strengthModifier", () -> getAbility(AbilityType.STRENGTH).modifier());
        registerValueTarget("dexterityModifier", () -> getAbility(AbilityType.DEXTERITY).modifier());
        registerValueTarget("constitutionModifier", () -> getAbility(AbilityType.CONSTITUTION).modifier());
        registerValueTarget("intelligenceModifier", () -> getAbility(AbilityType.INTELLIGENCE).modifier());
        registerValueTarget("wisdomModifier", () -> getAbility(AbilityType.WISDOM).modifier());
        registerValueTarget("charismaModifier", () -> getAbility(AbilityType.CHARISMA).modifier());

        registerValueTarget("armorClass", () -> getArmorClass().totalValue());
        registerValueTarget("touchArmorClass", () -> getArmorClass().touchValue());
        registerValueTarget("flatFootedArmorClass", () -> getArmorClass().flatFootedValue());
        registerValueTarget("initiative", () -> getInitiative().totalValue());
        registerValueTarget("maxHp", () -> getHitPoints().maxHp());
        registerValueTarget("currentHp", () -> getHitPoints().currentHp());
        registerValueTarget("temporaryHp", () -> getHitPoints().temporaryHp());
        registerValueTarget("nonlethalDamage", () -> getHitPoints().nonlethalDamage());
        registerValueTarget("fortitude", () -> getSavingThrow(SavingThrowType.FORTITUDE).totalValue());
        registerValueTarget("reflex", () -> getSavingThrow(SavingThrowType.REFLEX).totalValue());
        registerValueTarget("will", () -> getSavingThrow(SavingThrowType.WILL).totalValue());
    }

    private void registerDerivedTargetVariables()
    {
        targetManager.registerForbiddenVariables("strength", "strengthModifier");
        targetManager.registerForbiddenVariables("dexterity", "dexterityModifier");
        targetManager.registerForbiddenVariables("constitution", "constitutionModifier");
        targetManager.registerForbiddenVariables("intelligence", "intelligenceModifier");
        targetManager.registerForbiddenVariables("wisdom", "wisdomModifier");
        targetManager.registerForbiddenVariables("charisma", "charismaModifier");
        targetManager.registerForbiddenVariables("armorClass", "touchArmorClass", "flatFootedArmorClass");
    }

    private void registerAbilityModifierTarget(@NonNull String name, @NonNull AbilityType abilityType)
    {
        AbilityScore abilityScore = getAbilityScore(abilityType);
        targetManager.registerModifierTarget(name, new ModifierTarget()
        {
            @Override
            public void addModifier(@NonNull Modifier modifier)
            {
                abilityScore.addModifier(modifier);
            }

            @Override
            public void removeModifier(@NonNull UUID modifierId)
            {
                abilityScore.removeModifier(modifierId);
            }
        });
    }

    private void registerSavingThrowModifierTarget(@NonNull String name, @NonNull SavingThrowType savingThrowType)
    {
        SavingThrowEntry savingThrowEntry = getSavingThrowEntry(savingThrowType);
        targetManager.registerModifierTarget(name, new ModifierTarget()
        {
            @Override
            public void addModifier(@NonNull Modifier modifier)
            {
                savingThrowEntry.addModifier(modifier);
            }

            @Override
            public void removeModifier(@NonNull UUID modifierId)
            {
                savingThrowEntry.removeModifier(modifierId);
            }
        });
    }

    private void registerHitPointsDeltaTarget(@NonNull String name, @NonNull HitPointField field)
    {
        targetManager.registerDeltaTarget(name, delta -> hitPoints.change(field, delta));
    }

    private void registerValueTarget(@NonNull String name, @NonNull IntSupplier valueResolver)
    {
        targetManager.registerValueTarget(name, valueResolver);
    }
}
