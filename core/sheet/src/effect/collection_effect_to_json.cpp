#include "collection_effect.hpp"
#include "definition_json.hpp"
#include "golarion/view/damage_view.hpp"

namespace golarion
{
    namespace
    {
        using namespace definition_json;
        Json diceJson(const DamageDice &dice)
        {
            const auto v = dice.toView();
            return {{"diceCount", v.diceCount}, {"dieSize", v.dieSize}};
        }
        Json componentJson(const DamageComponent &component)
        {
            const auto e = component.toDefinition();
            return {{"id", e.id}, {"role", enumName(e.role)}, {"dice", diceJson(e.dice)},
                {"types", value(e.types)}, {"typeMode", enumName(e.typeMode)}, {"criticalRule", enumName(e.criticalRule)}, {"traits", value(e.traits)}};
        }
        Json selectorJson(const MovementSelector &e)
        {
            Json j{{"affectedByArmorOnly", e.affectedByArmorOnly}, {"affectedByLoadOnly", e.affectedByLoadOnly}};
            put(j, "movementType", e.type); put(j, "grantId", e.grantId); return j;
        }
        template<class Selector> Json selectorJson(const Selector &selector)
        {
            const auto e = selector.toDefinition();
            Json j = Json::object();
            if constexpr (requires { e.grantId; }) put(j, "grantId", e.grantId);
            if constexpr (requires { e.anyTags; }) put(j, "anyTags", e.anyTags);
            if constexpr (requires { e.excludedGrantIds; }) put(j, "excludedGrantIds", e.excludedGrantIds);
            if constexpr (requires { e.energy; }) put(j, "energy", e.energy);
            if constexpr (requires { e.actionId; }) put(j, "actionId", e.actionId);
            if constexpr (requires { e.categoryId; }) put(j, "categoryId", e.categoryId);
            if constexpr (requires { e.cost; }) put(j, "cost", e.cost);
            if constexpr (requires { e.requiredTags; }) put(j, "requiredTags", e.requiredTags);
            if constexpr (requires { e.allowedModes; }) put(j, "allowedModes", e.allowedModes);
            if constexpr (requires { e.forbiddenTags; }) put(j, "forbiddenTags", e.forbiddenTags);
            if constexpr (requires { e.allowedGrantIds; }) put(j, "allowedGrantIds", e.allowedGrantIds);
            return j;
        }
        Json progressionJson(const RoutineAttackProgressionDefinition &e)
        {
            Json j{{"kind", enumName(e.type)}, {"attackBonusAdjustmentExpression", e.attackBonusAdjustmentExpression}};
            put(j, "countExpression", e.countExpression); return j;
        }
        Json slotJson(const RoutineSlotEffectEntry &e)
        {
            Json j{{"id", e.id}, {"name", e.name}, {"selector", selectorJson(e.selector)},
                {"selectionMode", enumName(e.selectionMode)}, {"strikeUsage", enumName(e.strikeUsage)},
                {"baseAttackBonusAdjustmentExpression", e.baseAttackBonusAdjustmentExpression}, {"assignmentRequired", e.assignmentRequired},
                {"progressions", Json::array()}};
            for (const auto &p : e.progressions) j["progressions"].push_back(progressionJson(p));
            put(j, "damageAbilityRule", e.damageAbilityRuleOverride); put(j, "handUsage", e.handUsage); put(j, "handRole", e.handRole);
            return j;
        }
    }

    nlohmann::json collectionEffectToJson(const CollectionEffectDefinition &definition)
    {
        return std::visit([&](const auto &e) -> Json
        {
            using T = std::decay_t<decltype(e)>;
            Json j{{"id", definition.id}};
            if constexpr (std::is_same_v<T, EnergyResistanceDefinition>) j["type"] = "energyResistance";
            else if constexpr (std::is_same_v<T, EnergyResistanceAdjustmentDefinition>) j["type"] = "energyResistanceAdjustment";
            else if constexpr (std::is_same_v<T, EnergyVulnerabilityDefinition>) j["type"] = "energyVulnerability";
            else if constexpr (std::is_same_v<T, DamageReductionDefinition>) j["type"] = "damageReduction";
            else if constexpr (std::is_same_v<T, DamageReductionAdjustmentDefinition>) j["type"] = "damageReductionAdjustment";
            else if constexpr (std::is_same_v<T, ImmunityDefinition>) j["type"] = "immunity";
            else if constexpr (std::is_same_v<T, SpellResistanceDefinition>) j["type"] = "spellResistance";
            else if constexpr (std::is_same_v<T, SpellResistanceAdjustmentDefinition>) j["type"] = "spellResistanceAdjustment";
            else if constexpr (std::is_same_v<T, FastHealingDefinition>) j["type"] = "fastHealing";
            else if constexpr (std::is_same_v<T, RegenerationDefinition>) j["type"] = "regeneration";
            else if constexpr (std::is_same_v<T, SizeReplacementDefinition>) j["type"] = "sizeReplacement";
            else if constexpr (std::is_same_v<T, SizeAdjustmentDefinition>) j["type"] = "sizeAdjustment";
            else if constexpr (std::is_same_v<T, CarryingCapacityMultiplierDefinition>) j["type"] = "carryingCapacityMultiplier";
            else if constexpr (std::is_same_v<T, MovementAdjustmentDefinition>) j["type"] = "movementAdjustment";
            else if constexpr (std::is_same_v<T, RunAdjustmentDefinition>) j["type"] = "runAdjustment";
            else if constexpr (std::is_same_v<T, MovementBoundedBonus>) j["type"] = "movementBoundedBonus";
            else if constexpr (std::is_same_v<T, ArmorClassAbilityReplacementDefinition>) j["type"] = "armorClassAbilityReplacement";
            else if constexpr (std::is_same_v<T, ArmorClassAbilitySuppressionDefinition>) j["type"] = "armorClassAbilitySuppression";
            else if constexpr (std::is_same_v<T, ArmorClassDexterityRetention>) j["type"] = "armorClassDexterityRetention";
            else if constexpr (std::is_same_v<T, MaximumDexterityLimitDefinition>) j["type"] = "maximumDexterityLimit";
            else if constexpr (std::is_same_v<T, ArmorCheckPenaltyDefinition>) j["type"] = "armorCheckPenalty";
            else if constexpr (std::is_same_v<T, SkillAbilityReplacementDefinition>) j["type"] = "skillAbilityReplacement";
            else if constexpr (std::is_same_v<T, SkillClassSkillGrantDefinition>) j["type"] = "classSkillGrant";
            else if constexpr (std::is_same_v<T, InitiativeAbilityReplacementDefinition>) j["type"] = "initiativeAbilityReplacement";
            else if constexpr (std::is_same_v<T, SavingThrowAbilityReplacementDefinition>) j["type"] = "savingThrowAbilityReplacement";
            else if constexpr (std::is_same_v<T, TemporaryHitPointGrant>) j["type"] = "temporaryHitPoints";
            else if constexpr (std::is_same_v<T, ConditionEntryDefinition>) j["type"] = "conditionEntry";
            else if constexpr (std::is_same_v<T, AbilityScoreReplacementEffectEntry>) j["type"] = "abilityReplacement";
            else if constexpr (std::is_same_v<T, CombatManeuverAbilityReplacementDefinition>) j["type"] = "combatManeuverAbilityReplacement";
            else if constexpr (std::is_same_v<T, CombatManeuverDefenseDexteritySuppressionDefinition>) j["type"] = "combatManeuverDexteritySuppression";
            else if constexpr (std::is_same_v<T, AttackRoutineEffectEntry>) j["type"] = "attackRoutine";
            else if constexpr (std::is_same_v<T, RoutineProgressionEffectEntry>) j["type"] = "routineProgressionGrant";
            else if constexpr (std::is_same_v<T, RoutineAttackBonusAdjustmentDefinition>) j["type"] = "routineAttackBonusAdjustment";
            else if constexpr (std::is_same_v<T, ActionCategoryDefinition>) j["type"] = "actionCategory";
            else if constexpr (std::is_same_v<T, ActionDefinition>) j["type"] = "actionGrant";
            else if constexpr (std::is_same_v<T, ActionInhibitionDefinition>) j["type"] = "actionInhibition";
            else if constexpr (std::is_same_v<T, ActionNoteDefinition>) j["type"] = "actionNote";
            else if constexpr (std::is_same_v<T, ActionCostReplacementDefinition>) j["type"] = "actionCostReplacement";
            else if constexpr (std::is_same_v<T, StrikeGrantDefinition>) j["type"] = "strikeGrant";
            else if constexpr (std::is_same_v<T, DamageComponentGrantDefinition>) j["type"] = "damageComponentGrant";
            else if constexpr (std::is_same_v<T, CriticalAdjustmentDefinition>) j["type"] = "criticalAdjustment";
            else if constexpr (std::is_same_v<T, AttackAbilityReplacementDefinition>) j["type"] = "attackAbilityReplacement";
            else if constexpr (std::is_same_v<T, DamageAbilityReplacementDefinition>) j["type"] = "damageAbilityReplacement";
            else if constexpr (std::is_same_v<T, DamageDiceAdjustmentDefinition>) j["type"] = "damageDiceAdjustment";
            else if constexpr (std::is_same_v<T, AttackDistanceAdjustmentDefinition>) j["type"] = "attackDistanceAdjustment";
            else if constexpr (std::is_same_v<T, AttackRequirementDefinition>) j["type"] = "attackRequirement";
            else if constexpr (std::is_same_v<T, AttackDefenseReplacementDefinition>) j["type"] = "attackDefenseReplacement";
            else if constexpr (std::is_same_v<T, StrikeWeaponWeightAdjustmentDefinition>) j["type"] = "strikeWeaponWeightAdjustment";
            else if constexpr (std::is_same_v<T, StrikeUsageAdjustmentDefinition>) j["type"] = "strikeUsageAdjustment";
            else static_assert(sizeof(T) == 0, "missing collection effect serializer");

            if constexpr (requires { e.name; }) put(j, "name", e.name);
            if constexpr (requires { e.targetId; }) put(j, "targetId", e.targetId);
            if constexpr (requires { e.description; }) put(j, "description", e.description);
            if constexpr (requires { e.expression; }) put(j, "expression", e.expression);
            if constexpr (requires { e.tags; }) put(j, "tags", e.tags);
            if constexpr (requires { e.energy; }) put(j, "energy", e.energy);
            if constexpr (requires { e.stackingGroup; }) put(j, "stackingGroup", e.stackingGroup);
            if constexpr (requires { e.applicability; }) put(j, "applicability", e.applicability);
            if constexpr (requires { e.interruption; }) put(j, "interruption", e.interruption);
            if constexpr (requires { e.canBeLowered; }) put(j, "canBeLowered", e.canBeLowered);
            if constexpr (requires { e.category; }) put(j, "category", e.category);
            if constexpr (requires { e.acceptsAdjustments; }) put(j, "acceptsAdjustments", e.acceptsAdjustments);
            if constexpr (requires { e.steps; }) put(j, "steps", e.steps);
            if constexpr (requires { e.numerator; }) put(j, "numerator", e.numerator);
            if constexpr (requires { e.denominator; }) put(j, "denominator", e.denominator);
            if constexpr (requires { e.bonusType; }) put(j, "bonusType", e.bonusType);
            if constexpr (requires { e.maximumPercent; }) put(j, "maximumPercent", e.maximumPercent);
            if constexpr (requires { e.cause; }) put(j, "cause", e.cause);
            if constexpr (requires { e.causes; }) put(j, "causes", e.causes);
            if constexpr (requires { e.conditionId; }) put(j, "conditionId", e.conditionId);
            if constexpr (requires { e.severity; }) put(j, "severity", e.severity);
            if constexpr (requires { e.contributesToEscalation; }) put(j, "contributesToEscalation", e.contributesToEscalation);
            if constexpr (requires { e.parameter; }) put(j, "parameter", e.parameter);
            if constexpr (requires { e.stage; }) put(j, "stage", e.stage);
            if constexpr (requires { e.categoryId; }) put(j, "categoryId", e.categoryId);
            if constexpr (requires { e.actionId; }) put(j, "actionId", e.actionId);
            if constexpr (requires { e.text; }) put(j, "text", e.text);
            if constexpr (requires { e.cost; }) put(j, "cost", e.cost);
            if constexpr (requires { e.reason; }) put(j, "reason", e.reason);
            if constexpr (requires { e.mode; }) put(j, "mode", e.mode);
            if constexpr (requires { e.usageChannels; }) put(j, "usageChannels", e.usageChannels);
            if constexpr (requires { e.damageAbilityRule; }) put(j, "damageAbilityRule", e.damageAbilityRule);
            if constexpr (requires { e.criticalThreatMinimum; }) put(j, "criticalThreatMinimum", e.criticalThreatMinimum);
            if constexpr (requires { e.criticalMultiplier; }) put(j, "criticalMultiplier", e.criticalMultiplier);
            if constexpr (requires { e.naturalAttackClassification; }) put(j, "naturalAttackClassification", e.naturalAttackClassification);
            if constexpr (requires { e.weaponWeight; }) put(j, "weaponWeight", e.weaponWeight);
            if constexpr (requires { e.purpose; }) put(j, "purpose", e.purpose);
            if constexpr (requires { e.usage; }) put(j, "usage", e.usage);
            if constexpr (requires { e.attackPenaltyExpression; }) put(j, "attackPenaltyExpression", e.attackPenaltyExpression);
            if constexpr (requires { e.maximumMultiplier; }) put(j, "maximumMultiplier", e.maximumMultiplier);
            if constexpr (requires { e.condition; }) put(j, "condition", e.condition);
            if constexpr (requires { e.targetResourceName; }) put(j, "resource", e.targetResourceName);
            if constexpr (requires { e.abilityType; }) put(j, "ability", e.abilityType);
            if constexpr (requires { e.ability; }) put(j, "ability", e.ability);
            if constexpr (requires { e.savingThrowType; }) put(j, "savingThrow", e.savingThrowType);
            if constexpr (requires { e.amountExpression; }) put(j, "expression", e.amountExpression);
            if constexpr (requires { e.targetRoutineId; }) put(j, "routineId", e.targetRoutineId);
            if constexpr (requires { e.targetSlotId; }) put(j, "slotId", e.targetSlotId);
            if constexpr (requires { e.targetRole; }) put(j, "role", e.targetRole);
            if constexpr (requires { e.targetOrigin; }) put(j, "origin", e.targetOrigin);
            if constexpr (requires { e.targetComponentGrantId; }) put(j, "componentGrantId", e.targetComponentGrantId);
            if constexpr (requires { e.targetComponentId; }) put(j, "componentId", e.targetComponentId);
            if constexpr (requires { e.defenseType; }) put(j, "defense", e.defenseType);
            if constexpr (requires { e.damageAbility; }) put(j, "ability", e.damageAbility);
            if constexpr (requires { e.type; })
            {
                if constexpr (std::is_same_v<T, MaximumDexterityLimitDefinition>) put(j, "limitType", e.type);
                else put(j, "adjustment", e.type);
            }
            if constexpr (requires { e.selector; }) j["selector"] = selectorJson(e.selector);
            if constexpr (requires { e.requirement; })
            { j["expression"] = e.requirement.expression(); j["failureReason"] = e.requirement.failureReason(); }
            if constexpr (std::is_same_v<T, RoutineAttackBonusAdjustmentDefinition>)
            {
                j["requirements"] = Json::array();
                for (const auto &requirement : e.requirements)
                {
                    const auto r = requirement.toDefinition();
                    Json result{{"slotId", r.slotId}};
                    put(result, "strikeGrantId", r.strikeGrantId); put(result, "requiredTag", r.requiredTag);
                    put(result, "purpose", r.weaponWeightPurpose); put(result, "weaponWeight", r.weaponWeight);
                    j["requirements"].push_back(std::move(result));
                }
            }
            else if constexpr (requires { e.requirements; }) j["requirements"] = requirements(e.requirements);
            if constexpr (requires { e.slots; })
            {
                j["slots"] = Json::array();
                for (const auto &slot : e.slots) j["slots"].push_back(slotJson(slot));
            }
            if constexpr (requires { e.progression; }) j["progression"] = progressionJson(e.progression);
            if constexpr (requires { e.duration; }) if (e.duration) j["durationRounds"] = e.duration->roundCount();
            if constexpr (requires { e.setDice; }) if (e.setDice) j["dice"] = diceJson(*e.setDice);
            if constexpr (requires { e.component; }) j["component"] = componentJson(e.component);
            if constexpr (requires { e.damageComponents; })
            {
                j["damageComponents"] = Json::array();
                for (const auto &component : e.damageComponents) j["damageComponents"].push_back(componentJson(component));
            }
            if constexpr (requires { e.reach; })
            {
                if (e.reach) j["reach"] = {{"minimumUnits", e.reach->minimumUnits}, {"maximumUnits", e.reach->maximumUnits}};
                if (e.range) j["range"] = {{"incrementUnits", e.range->incrementUnits}, {"maximumIncrements", e.range->maximumIncrements}, {"penaltyPerAdditionalIncrement", e.range->penaltyPerAdditionalIncrement}};
            }
            if constexpr (requires { e.bypass; })
            {
                j["bypass"] = Json::array();
                for (const auto &alternative : e.bypass.anyOf) j["bypass"].push_back(value(alternative.allOf));
                j["stacksWithTags"] = e.stacksWithTags;
            }
            return j;
        }, definition.entry);
    }
}
