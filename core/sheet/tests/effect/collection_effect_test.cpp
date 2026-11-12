#include "../fixtures/definition_roundtrip.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/resource/resource_manager.hpp"
#include <nlohmann/json.hpp>
#include <cassert>

namespace
{
    using namespace golarion;
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
    template<class Entry> void check(std::string_view endpoint, const char *json)
    {
        ResourceManager resources;
        int count = 0;
        resources.registerCollectionResource<Entry>(endpoint, [&](Entry) { ++count; }, [&](std::string_view id) { assert(id == "runtime"); --count; });
        auto value = nlohmann::json::parse(json);
        value["id"] = "local";
        auto effect = roundTripEffect(value);
        assert(effectId(effect) == "local");
        assertJsonContains(effectDefinitionToJson(effect), value);
        auto cleanup = compileEffect(effect, {"runtime", "Fonte"})(resources);
        assert(count == 1);
        cleanup();
        assert(count == 0);
        if (value.contains("expression"))
        {
            value["expression"] = " ";
            rejects([&] { roundTripEffect(value); });
        }
    }
}

int main()
{
    using namespace golarion;
    check<RoutineProgressionGrant>(AttackRoutineProgressionGrantsResource, R"({"type":"routineProgressionGrant","routineId":"base.fullAttack","slotId":"weapon","progression":{"kind":"fixed","countExpression":"1"}})");
    check<RoutineAttackBonusAdjustment>(AttackRoutineBonusAdjustmentsResource, R"({"type":"routineAttackBonusAdjustment","routineId":"base.fullAttack","slotId":"weapon","expression":"-2","requirements":[{"slotId":"weapon","requiredTag":"weapon"}]})");
    check<AttackRoutine>(AttackRoutinesResource, R"({"type":"attackRoutine","name":"Test","actionId":"attack","slots":[{"id":"main","name":"Attacco","selectionMode":"chooseOne","strikeUsage":"default","progressions":[{"kind":"fixed","countExpression":"1"}]}]})");
    check<EnergyResistance>(EnergyResistanceGrantsResource, R"({"type":"energyResistance","energy":"fire","expression":"5"})");
    check<EnergyResistanceAdjustment>(EnergyResistanceAdjustmentsResource, R"({"type":"energyResistanceAdjustment","expression":"5","stackingGroup":"test"})");
    check<EnergyVulnerability>(EnergyVulnerabilityGrantsResource, R"({"type":"energyVulnerability","energy":"cold"})");
    check<DamageReduction>(DamageReductionGrantsResource, R"({"type":"damageReduction","expression":"5","bypass":[["magic","silver"],["good"]]})");
    check<DamageReductionAdjustment>(DamageReductionAdjustmentsResource, R"({"type":"damageReductionAdjustment","expression":"2","stackingGroup":"test"})");
    check<Immunity>(ImmunityGrantsResource, R"({"type":"immunity","targetId":"sleep","name":"Sonno"})");
    check<SpellResistance>(SpellResistanceGrantsResource, R"({"type":"spellResistance","expression":"10"})");
    check<SpellResistanceAdjustment>(SpellResistanceAdjustmentsResource, R"({"type":"spellResistanceAdjustment","expression":"2","stackingGroup":"test"})");
    check<FastHealing>(FastHealingGrantsResource, R"({"type":"fastHealing","expression":"3"})");
    check<Regeneration>(RegenerationGrantsResource, R"({"type":"regeneration","expression":"5","interruption":"Fuoco"})");
    check<SizeReplacement>(SizeReplacementsResource, R"({"type":"sizeReplacement","category":"large"})");
    check<SizeAdjustment>(SizeAdjustmentsResource, R"({"type":"sizeAdjustment","steps":1})");
    check<CarryingCapacityMultiplier>(CarryingCapacityMultipliersResource, R"({"type":"carryingCapacityMultiplier","stackingGroup":"test","numerator":2,"denominator":1})");
    check<MovementAdjustment>(MovementAdjustmentsResource, R"({"type":"movementAdjustment","adjustment":"speedMultiplier","description":"Metà","expression":"50"})");
    check<RunAdjustment>(RunAdjustmentsResource, R"({"type":"runAdjustment","adjustment":"increase","description":"Corsa","expression":"1","stackingGroup":"test"})");
    check<MovementBoundedBonus>(MovementBoundedBonusesResource, R"({"type":"movementBoundedBonus","description":"Velocità","bonusType":"enhancement","expression":"6"})");
    check<ArmorClassAbilityReplacement>(ArmorClassAbilityReplacementsResource, R"({"type":"armorClassAbilityReplacement","ability":"wis"})");
    check<ArmorClassAbilitySuppression>(ArmorClassAbilitySuppressionsResource, R"({"type":"armorClassAbilitySuppression","cause":"invisibleAttacker"})");
    check<ArmorClassDexterityRetention>(ArmorClassDexterityRetentionsResource, R"({"type":"armorClassDexterityRetention","causes":["flatFooted"]})");
    check<MaximumDexterityLimit>(MaximumDexterityLimitsResource, R"({"type":"maximumDexterityLimit","limitType":"armor","expression":"3"})");
    check<ArmorCheckPenalty>(ArmorCheckPenaltiesResource, R"({"type":"armorCheckPenalty","expression":"2"})");
    check<SkillAbilityReplacement>(SkillAbilityReplacementsResource, R"({"type":"skillAbilityReplacement","resource":"skill.perception","ability":"int"})");
    check<SkillClassSkillGrant>(SkillClassSkillGrantsResource, R"({"type":"classSkillGrant","resource":"skill.perception"})");
    check<InitiativeAbilityReplacement>(InitiativeAbilityReplacementsResource, R"({"type":"initiativeAbilityReplacement","ability":"int"})");
    check<SavingThrowAbilityReplacement>(SavingThrowAbilityReplacementsResource, R"({"type":"savingThrowAbilityReplacement","ability":"cha","savingThrow":"will"})");
    check<TemporaryHitPointGrant>(TemporaryHitPointsResource, R"({"type":"temporaryHitPoints","expression":"5","durationRounds":10})");
    check<ConditionEntry>(ConditionEntriesResource, R"({"type":"conditionEntry","conditionId":"fatigue"})");
    check<AbilityReplacement>(abilityReplacementsResourceName(AbilityType::Strength), R"({"type":"abilityReplacement","ability":"str","stage":"final","expression":"0"})");
    check<CombatManeuverAbilityReplacement>(CombatManeuverAbilityReplacementsResource, R"({"type":"combatManeuverAbilityReplacement","resource":"combatManeuver.bonus.all","ability":"dex"})");
    check<CombatManeuverDefenseDexteritySuppression>(CombatManeuverDefenseDexteritySuppressionsResource, R"({"type":"combatManeuverDexteritySuppression"})");
    check<ActionCategory>(ActionCategoriesResource, R"({"type":"actionCategory","name":"Test"})");
    check<Action>(ActionGrantsResource, R"({"type":"actionGrant","name":"Test","description":"Test","categoryId":"combat","cost":"standard"})");
    check<ActionInhibition>(ActionInhibitionsResource, R"({"type":"actionInhibition","reason":"Test","selector":{"cost":"fullRound"}})");
    check<ActionNote>(ActionNotesResource, R"({"type":"actionNote","actionId":"run","text":"Test"})");
    check<ActionCostReplacement>(ActionCostReplacementsResource, R"({"type":"actionCostReplacement","actionId":"run","cost":"move"})");
    check<AttackAbilityReplacement>(AttackAbilityReplacementsResource, R"({"type":"attackAbilityReplacement","resource":"attack.all","ability":"dex"})");
    check<DamageAbilityReplacement>(DamageAbilityReplacementsResource, R"({"type":"damageAbilityReplacement","resource":"damage.all","ability":"dex"})");
    check<CriticalAdjustment>(CriticalAdjustmentsResource, R"({"type":"criticalAdjustment","resource":"attack.all","adjustment":"threatRangeMultiplier","expression":"2"})");
    check<AttackRequirement>(AttackRequirementsResource, R"({"type":"attackRequirement","resource":"attack.all","expression":"1","failureReason":"Test"})");
    check<AttackDefenseReplacement>(AttackDefenseReplacementsResource, R"({"type":"attackDefenseReplacement","resource":"attack.all","defense":"touch"})");
    check<AttackDistanceAdjustment>(AttackDistanceAdjustmentsResource, R"({"type":"attackDistanceAdjustment","resource":"attack.rangeIncrement","adjustment":"multiplier","expression":"2"})");
    check<StrikeWeaponWeightAdjustment>(StrikeWeaponWeightAdjustmentsResource, R"({"type":"strikeWeaponWeightAdjustment","resource":"attack.all","purpose":"twoWeaponFighting","weaponWeight":"light"})");
    check<StrikeUsageAdjustment>(StrikeUsageAdjustmentsResource, R"({"type":"strikeUsageAdjustment","resource":"attack.all","usage":"naturalSecondary","attackPenaltyExpression":"2"})");
    check<DamageDiceAdjustment>(DamageDiceAdjustmentsResource, R"({"type":"damageDiceAdjustment","resource":"damage.all","role":"base","origin":"any","adjustment":"progressionSteps","expression":"1"})");
    check<DamageComponentGrant>(DamageComponentGrantsResource, R"({"type":"damageComponentGrant","resource":"damage.all","component":{"id":"fire","role":"additional","dice":{"diceCount":1,"dieSize":6},"types":["fire"],"typeMode":"all","criticalRule":"notMultiplied"}})");
    check<StrikeGrant>(StrikeGrantsResource, R"({"type":"strikeGrant","name":"Artiglio","mode":"melee","tags":["natural"],"naturalAttackClassification":"primary","usageChannels":["hand"],"ability":"str","damageAbilityRule":"full","reach":{"maximumUnits":1},"damageComponents":[{"id":"base","role":"base","dice":{"diceCount":1,"dieSize":6},"types":["slashing"],"typeMode":"all","criticalRule":"multiplied"}]})");
    for (const auto *json : {
        R"({"id":"x","type":"energyResistance","energy":"laser","expression":"5"})",
        R"({"id":"x","type":"movementBoundedBonus","bonusType":"enhancement","description":"Test","expression":"6","maximumPercent":99})",
        R"({"id":"x","type":"sizeAdjustment","steps":1.5})",
        R"({"id":"x","type":"conditionEntry","conditionId":"fatigue","severity":1.5})",
        R"({"id":"x","type":"runAdjustment","adjustment":"unknown","description":"Test","expression":"1","stackingGroup":"test"})"})
        rejects([&] { roundTripEffect(nlohmann::json::parse(json)); });
}
