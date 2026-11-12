#include "collection_effect.hpp"
#include "golarion/resource/resource_manager.hpp"

namespace
{
    using namespace golarion;
    template<class Entry> EffectApply registration(std::string_view endpoint, Entry entry, const EffectContext &context)
    {
        return [resource = std::string(endpoint), entry = std::move(entry), id = context.instanceId](ResourceManager &resources)
        {
            EffectCleanup cleanup = [&resources, resource, id] { resources.removeFromCollection(resource, id); };
            resources.addToCollection(resource, entry);
            return cleanup;
        };
    }

    template<class Definition> EffectApply compile(Definition definition, const EffectContext &context)
    {
        definition.id = context.instanceId;
        if constexpr (requires { definition.source; }) definition.source = context.source;
        if constexpr (std::is_same_v<Definition, StrikeGrantDefinition>)
            for (auto &component : definition.damageComponents) component = component.withSource(context.source);
        if constexpr (std::is_same_v<Definition, DamageComponentGrantDefinition>) definition.component = definition.component.withSource(context.source);
        if constexpr (std::is_same_v<Definition, AbilityScoreReplacementEffectEntry>)
            return registration(abilityReplacementsResourceName(definition.ability), AbilityReplacement(definition), context);
        else if constexpr (std::is_same_v<Definition, RoutineProgressionEffectEntry>)
            return registration(AttackRoutineProgressionGrantsResource, RoutineProgressionGrant({definition.id, definition.source,
                definition.targetRoutineId, definition.targetSlotId, RoutineAttackProgression(definition.progression)}), context);
        else if constexpr (std::is_same_v<Definition, AttackRoutineEffectEntry>)
        {
            std::vector<RoutineSlot> slots;
            for (const auto &slot : definition.slots)
            {
                std::vector<RoutineAttackProgression> progressions;
                for (const auto &progression : slot.progressions) progressions.emplace_back(progression);
                slots.emplace_back(RoutineSlotDefinition{slot.id, slot.name, slot.selector, slot.selectionMode, slot.strikeUsage,
                    std::move(progressions), slot.damageAbilityRuleOverride, slot.baseAttackBonusAdjustmentExpression, slot.handUsage, slot.handRole, slot.assignmentRequired});
            }
            return registration(AttackRoutinesResource, AttackRoutine({definition.id, definition.source, definition.name, definition.actionId, std::move(slots)}), context);
        }
        else
#define ENTRY(DefinitionType, EntryType, Endpoint) \
        if constexpr (std::is_same_v<Definition, DefinitionType>) return registration(Endpoint, EntryType(definition), context); else
        ENTRY(EnergyResistanceDefinition, EnergyResistance, EnergyResistanceGrantsResource)
        ENTRY(EnergyResistanceAdjustmentDefinition, EnergyResistanceAdjustment, EnergyResistanceAdjustmentsResource)
        ENTRY(EnergyVulnerabilityDefinition, EnergyVulnerability, EnergyVulnerabilityGrantsResource)
        ENTRY(DamageReductionDefinition, DamageReduction, DamageReductionGrantsResource)
        ENTRY(DamageReductionAdjustmentDefinition, DamageReductionAdjustment, DamageReductionAdjustmentsResource)
        ENTRY(ImmunityDefinition, Immunity, ImmunityGrantsResource)
        ENTRY(SpellResistanceDefinition, SpellResistance, SpellResistanceGrantsResource)
        ENTRY(SpellResistanceAdjustmentDefinition, SpellResistanceAdjustment, SpellResistanceAdjustmentsResource)
        ENTRY(FastHealingDefinition, FastHealing, FastHealingGrantsResource)
        ENTRY(RegenerationDefinition, Regeneration, RegenerationGrantsResource)
        ENTRY(SizeReplacementDefinition, SizeReplacement, SizeReplacementsResource)
        ENTRY(SizeAdjustmentDefinition, SizeAdjustment, SizeAdjustmentsResource)
        ENTRY(CarryingCapacityMultiplierDefinition, CarryingCapacityMultiplier, CarryingCapacityMultipliersResource)
        ENTRY(MovementAdjustmentDefinition, MovementAdjustment, MovementAdjustmentsResource)
        ENTRY(RunAdjustmentDefinition, RunAdjustment, RunAdjustmentsResource)
        ENTRY(MovementBoundedBonus, MovementBoundedBonus, MovementBoundedBonusesResource)
        ENTRY(ArmorClassAbilityReplacementDefinition, ArmorClassAbilityReplacement, ArmorClassAbilityReplacementsResource)
        ENTRY(ArmorClassAbilitySuppressionDefinition, ArmorClassAbilitySuppression, ArmorClassAbilitySuppressionsResource)
        ENTRY(ArmorClassDexterityRetention, ArmorClassDexterityRetention, ArmorClassDexterityRetentionsResource)
        ENTRY(MaximumDexterityLimitDefinition, MaximumDexterityLimit, MaximumDexterityLimitsResource)
        ENTRY(ArmorCheckPenaltyDefinition, ArmorCheckPenalty, ArmorCheckPenaltiesResource)
        ENTRY(SkillAbilityReplacementDefinition, SkillAbilityReplacement, SkillAbilityReplacementsResource)
        ENTRY(SkillClassSkillGrantDefinition, SkillClassSkillGrant, SkillClassSkillGrantsResource)
        ENTRY(InitiativeAbilityReplacementDefinition, InitiativeAbilityReplacement, InitiativeAbilityReplacementsResource)
        ENTRY(SavingThrowAbilityReplacementDefinition, SavingThrowAbilityReplacement, SavingThrowAbilityReplacementsResource)
        ENTRY(TemporaryHitPointGrant, TemporaryHitPointGrant, TemporaryHitPointsResource)
        ENTRY(ConditionEntryDefinition, ConditionEntry, ConditionEntriesResource)
        ENTRY(RoutineAttackBonusAdjustmentDefinition, RoutineAttackBonusAdjustment, AttackRoutineBonusAdjustmentsResource)
        ENTRY(CombatManeuverAbilityReplacementDefinition, CombatManeuverAbilityReplacement, CombatManeuverAbilityReplacementsResource)
        ENTRY(CombatManeuverDefenseDexteritySuppressionDefinition, CombatManeuverDefenseDexteritySuppression, CombatManeuverDefenseDexteritySuppressionsResource)
        ENTRY(ActionCategoryDefinition, ActionCategory, ActionCategoriesResource)
        ENTRY(ActionDefinition, Action, ActionGrantsResource)
        ENTRY(ActionInhibitionDefinition, ActionInhibition, ActionInhibitionsResource)
        ENTRY(ActionNoteDefinition, ActionNote, ActionNotesResource)
        ENTRY(ActionCostReplacementDefinition, ActionCostReplacement, ActionCostReplacementsResource)
        ENTRY(StrikeGrantDefinition, StrikeGrant, StrikeGrantsResource)
        ENTRY(DamageComponentGrantDefinition, DamageComponentGrant, DamageComponentGrantsResource)
        ENTRY(CriticalAdjustmentDefinition, CriticalAdjustment, CriticalAdjustmentsResource)
        ENTRY(AttackAbilityReplacementDefinition, AttackAbilityReplacement, AttackAbilityReplacementsResource)
        ENTRY(DamageAbilityReplacementDefinition, DamageAbilityReplacement, DamageAbilityReplacementsResource)
        ENTRY(DamageDiceAdjustmentDefinition, DamageDiceAdjustment, DamageDiceAdjustmentsResource)
        ENTRY(AttackDistanceAdjustmentDefinition, AttackDistanceAdjustment, AttackDistanceAdjustmentsResource)
        ENTRY(AttackRequirementDefinition, AttackRequirement, AttackRequirementsResource)
        ENTRY(AttackDefenseReplacementDefinition, AttackDefenseReplacement, AttackDefenseReplacementsResource)
        ENTRY(StrikeWeaponWeightAdjustmentDefinition, StrikeWeaponWeightAdjustment, StrikeWeaponWeightAdjustmentsResource)
        ENTRY(StrikeUsageAdjustmentDefinition, StrikeUsageAdjustment, StrikeUsageAdjustmentsResource)
        { static_assert(!sizeof(Definition), "unmapped collection effect"); }
#undef ENTRY
    }
}

namespace golarion
{
    EffectApply compileCollectionEffect(CollectionEffectDefinition definition, EffectContext context)
    {
        return std::visit([&](auto entry) { return compile(std::move(entry), context); }, std::move(definition.entry));
    }
}
