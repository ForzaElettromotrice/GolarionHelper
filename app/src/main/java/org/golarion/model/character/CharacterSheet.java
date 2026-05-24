package org.golarion.model.character;

import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import org.golarion.model.api.*;
import org.golarion.model.character.ability.AbilityScore;
import org.golarion.model.character.ability.AbilityType;
import org.golarion.model.character.action.ActionSource;
import org.golarion.model.character.action.ActionSourceType;
import org.golarion.model.character.action.ApplySizeAction;
import org.golarion.model.character.action.ReverseAction;
import org.golarion.model.character.alignment.Alignment;
import org.golarion.model.character.armorclass.ArmorClassEntry;
import org.golarion.model.character.attack.AttackEntry;
import org.golarion.model.character.attack.AttackStats;
import org.golarion.model.character.attack.AttackType;
import org.golarion.model.character.attack.DamageType;
import org.golarion.model.character.equipment.Equipment;
import org.golarion.model.character.equipment.EquipmentContainerEntry;
import org.golarion.model.character.equipment.EquipmentEntry;
import org.golarion.model.character.hitpoints.HitPointField;
import org.golarion.model.character.hitpoints.HitPointsEntry;
import org.golarion.model.character.initiative.InitiativeEntry;
import org.golarion.model.character.modifier.*;
import org.golarion.model.character.savingthrow.SavingThrowEntry;
import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.character.size.CharacterSize;
import org.golarion.model.character.skill.SkillType;
import org.golarion.model.character.skill.Skills;
import org.golarion.model.character.speed.SpeedEntry;

import java.util.*;

public class CharacterSheet
{
    private static final ActionSource SIZE_ACTION_SOURCE = new ActionSource(ActionSourceType.SIZE, new UUID(0, 1));
    private static final ActionSource EQUIPMENT_CARRYING_LOAD_ACTION_SOURCE = new ActionSource(ActionSourceType.EQUIP, new UUID(0, 2));

    private final EnumMap<AbilityType, AbilityScore> abilityScores;
    private final EnumMap<SavingThrowType, SavingThrowEntry> savingThrows;
    private final ArmorClassEntry armorClass;
    private final HitPointsEntry hitPoints;
    private final InitiativeEntry initiative;
    private final Skills skills;
    private final AttackStats attackStats;
    private final Equipment equipment;
    private final SpeedEntry speed;
    private final TargetManager targetManager;
    private final Map<UUID, EffectGroup> effectGroups;
    private final Map<ActionSource, List<ReverseAction>> appliedActions;
    @Getter
    private String characterName;
    @Setter
    @Getter
    private Alignment alignment;
    @Getter
    private CharacterSize size;

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
        this.equipment = new Equipment();
        this.equipment.setStrengthResolver(() -> getAbilityScore(AbilityType.STRENGTH).getTotalValue());
        this.speed = new SpeedEntry();
        this.alignment = Alignment.TRUE_NEUTRAL;
        this.size = CharacterSize.MEDIUM;
        setCharacterName(characterName);

        this.targetManager = new TargetManager();
        this.effectGroups = new LinkedHashMap<>();
        this.appliedActions = new LinkedHashMap<>();

        registerAbilityTargets();
        registerSavingThrowTargets();
        registerModifierTargets();
        registerDeltaTargets();
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

    public void setSize(@NonNull CharacterSize size)
    {
        if (this.size == size)
        {
            return;
        }

        CharacterSize previousSize = this.size;
        reverseActions(SIZE_ACTION_SOURCE);
        this.size = size;

        try
        {
            appliedActions.put(SIZE_ACTION_SOURCE, List.of(new ApplySizeAction(size).apply(this)));
        } catch (RuntimeException exception)
        {
            this.size = previousSize;
            appliedActions.put(SIZE_ACTION_SOURCE, List.of(new ApplySizeAction(previousSize).apply(this)));
            throw exception;
        }
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
                mod.getValue();
            } catch (IllegalArgumentException exception)
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

    public void setMaxDexterityBonus(int maxDexterityBonus)
    {
        armorClass.setMaxDexterityBonus(maxDexterityBonus);
    }

    public void changeCarryingCapacityMultiplier(double multiplier)
    {
        equipment.changeCarryingCapacityMultiplier(multiplier);
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

    public void setBaseSpeedUnits(int baseUnits)
    {
        speed.setBaseUnits(baseUnits);
    }

    public SpeedData getSpeed()
    {
        return speed.toData();
    }

    public UUID addEquipmentContainer(@NonNull String name)
    {
        return equipment.addContainer(name).getId();
    }

    public void removeEquipmentContainer(@NonNull UUID containerId)
    {
        equipment.removeContainer(containerId);
    }

    public void addEquipmentItem(@NonNull UUID containerId, @NonNull EquipmentEntry item)
    {
        applyCarryingLoadAction(equipment.addItem(containerId, item));
    }

    public void moveEquipmentItem(@NonNull UUID itemId, @NonNull UUID destinationContainerId)
    {
        applyCarryingLoadAction(equipment.moveItem(itemId, destinationContainerId));
    }

    public void removeEquipmentItem(@NonNull UUID itemId)
    {
        applyCarryingLoadAction(equipment.removeItem(itemId));
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

    public void setCombatManeuverBonusAbilityType(@NonNull AbilityType abilityType)
    {
        attackStats.setCombatManeuverBonusAbilityType(abilityType);
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

    private void applyCarryingLoadAction(@NonNull org.golarion.model.character.action.Action action)
    {
        reverseActions(EQUIPMENT_CARRYING_LOAD_ACTION_SOURCE);
        appliedActions.put(EQUIPMENT_CARRYING_LOAD_ACTION_SOURCE, List.of(action.apply(this)));
    }

    private void reverseActions(@NonNull ActionSource actionSource)
    {
        List<ReverseAction> reverseActions = appliedActions.remove(actionSource);
        if (reverseActions == null)
        {
            return;
        }

        reverseActions.forEach(ReverseAction::apply);
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
        armorClass.registerTargets(targetManager);
        initiative.registerTargets(targetManager);
        skills.registerModifierTargets(targetManager);
        attackStats.registerModifierTargets(targetManager);
        equipment.registerModifierTargets(targetManager);
        speed.registerTargets(targetManager);
    }

    private void registerDeltaTargets()
    {
        hitPoints.registerDeltaTargets(targetManager);
        equipment.registerDeltaTargets(targetManager);
    }

    private void registerAbilityTargets()
    {
        getAbilityScore(AbilityType.STRENGTH).registerTargets(targetManager, "strength");
        getAbilityScore(AbilityType.DEXTERITY).registerTargets(targetManager, "dexterity");
        getAbilityScore(AbilityType.CONSTITUTION).registerTargets(targetManager, "constitution");
        getAbilityScore(AbilityType.INTELLIGENCE).registerTargets(targetManager, "intelligence");
        getAbilityScore(AbilityType.WISDOM).registerTargets(targetManager, "wisdom");
        getAbilityScore(AbilityType.CHARISMA).registerTargets(targetManager, "charisma");
    }

    private void registerSavingThrowTargets()
    {
        getSavingThrowEntry(SavingThrowType.FORTITUDE).registerTargets(targetManager, "fortitude");
        getSavingThrowEntry(SavingThrowType.REFLEX).registerTargets(targetManager, "reflex");
        getSavingThrowEntry(SavingThrowType.WILL).registerTargets(targetManager, "will");
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
