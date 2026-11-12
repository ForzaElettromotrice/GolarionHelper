#include "golarion/character/character_sheet.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/spell_definition_json.hpp"
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
    EffectDefinition effect(const char *json) { return effectDefinitionFromJson(nlohmann::json::parse(json)); }
}

int main()
{
    using namespace golarion;
    auto definition = SpellDefinitionManager::instance().get("fireball");
    definition.id = "custom.buff";
    definition.maxTargetsExpression = "@level / 2";
    definition.effects = {
        effect(R"({"id":"bonus","type":"modifier","resource":"str","description":"Bonus","modifierType":"bonus","bonusType":"enhancement","expression":"@level","condition":"against evil"})"),
        effect(R"({"id":"speed","type":"movementBoundedBonus","description":"Velocità","bonusType":"enhancement","expression":"6","maximumPercent":200})"),
        effect(R"({"id":"attack","type":"routineProgressionGrant","routineId":"base.fullAttack","slotId":"weapon","progression":{"kind":"fixed","countExpression":"1","attackBonusAdjustmentExpression":"0"}})")
    };
    const auto original = spellDefinitionToJson(definition);
    assert(spellDefinitionToJson(spellDefinitionFromJson(original)) == original);
    auto legacy = original;
    legacy.erase("maxTargetsExpression");
    assert(spellDefinitionFromJson(legacy).maxTargetsExpression == "-1");
    auto malformed = original;
    malformed["maxTargetsExpression"] = 2;
    rejects([&] { spellDefinitionFromJson(malformed); });

    ResourceManager resources;
    int level = 8;
    resources.registerTarget("level", [&] { return level; });
    auto result = exportSpellCast(definition, resources);
    assert(result.maxTargets == 4 && !result.metamagic && result.effects.size() == 3);
    const auto &bonus = std::get<ModifierEffectDefinition>(result.effects[0]);
    assert(bonus.expression == "8" && bonus.condition == "against evil");
    const auto &speed = std::get<MovementBoundedBonus>(std::get<CollectionEffectDefinition>(result.effects[1]).entry);
    assert(speed.expression == "6" && speed.maximumPercent == 200);
    const auto &attack = std::get<RoutineProgressionEffectEntry>(std::get<CollectionEffectDefinition>(result.effects[2]).entry);
    assert(attack.progression.countExpression == "1" && attack.progression.attackBonusAdjustmentExpression == "0");
    level = 12;
    assert(bonus.expression == "8" && result.maxTargets == 4);
    assert(spellDefinitionToJson(definition) == original);

    // Exported values can be installed on an unrelated recipient with no caster targets.
    CharacterSheet recipient;
    recipient.addEnhancementGroup({.id = "cast", .name = "Incantesimo", .enhancements = result.effects});
    recipient.setEnhancementGroupActive("cast", false);
    recipient.removeEnhancementGroup("cast");

    // Nested effects and requirements use the same caster snapshot.
    auto nested = definition;
    nested.effects = {FavoredClassBonusEffectDefinition{.id = "nested", .name = "Bonus", .description = "Test",
        .effects = {ContributionEffectDefinition{"value", "test", "@level + 1"}}}};
    auto nestedResult = exportSpellCast(nested, resources);
    assert(std::get<ContributionEffectDefinition>(std::get<FavoredClassBonusEffectDefinition>(nestedResult.effects[0]).effects[0]).expression == "13");
    for (const auto *invalid : {"0", "-2", "@missing"})
    {
        auto bad = definition;
        bad.maxTargetsExpression = invalid;
        rejects([&] { exportSpellCast(bad, resources); });
    }

    SpellCatalog catalog;
    catalog.add(definition);
    auto broken = definition;
    broken.id = "custom.broken";
    broken.effects = {ContributionEffectDefinition{"bad", "test", "@missing"}};
    catalog.add(broken);
    auto unlimited = definition;
    unlimited.id = "custom.unlimited";
    unlimited.maxTargetsExpression = "-1";
    catalog.add(unlimited);
    auto badTargets = definition;
    badTargets.id = "custom.badTargets";
    badTargets.maxTargetsExpression = "0";
    catalog.add(badTargets);
    const SpellSlotId slot{"base", 3, 0};
    PreparedSpellcasting prepared(resources, "prepared", CharacterClassType::Wizard, {}, &catalog);
    SpontaneousSpellcasting spontaneous(resources, "spontaneous", CharacterClassType::Wizard, {}, &catalog);
    FlexibleSpellcasting flexible(resources, "flexible", CharacterClassType::Wizard, {}, &catalog);
    for (auto *casting : std::vector<Spellcasting *>{&prepared, &spontaneous, &flexible})
    {
        const auto id = casting == &prepared ? "prepared" : casting == &spontaneous ? "spontaneous" : "flexible";
        resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"base", "Test", {{3, 1}}});
        resources.addToCollection(repertoireGrantsResource(id), SpellRepertoireGrant{"known", "Test", {definition.id, broken.id, unlimited.id, badTargets.id}});
    }
    resources.addToCollection(preparationSlotsResource("flexible"), SpellSlotGrant{"base", "Test", {{3, 1}}});
    prepared.prepare(slot, {definition.id});
    auto preparedResult = prepared.consume(slot);
    assert(preparedResult.maxTargets == 6 && preparedResult.metamagic->spellId == definition.id);
    assert(preparedResult.effects.size() == 3);
    auto spontaneousResult = spontaneous.consume(slot, {unlimited.id});
    assert(spontaneousResult.maxTargets == -1 && spontaneousResult.metamagic->spellId == unlimited.id);
    flexible.prepare(slot, {definition.id});
    assert(flexible.consume(slot, {slot}).effects.size() == 3);

    // A failed export never rolls back a valid cast, including invalid target counts.
    prepared.clear(slot);
    prepared.prepare(slot, {broken.id});
    const auto metamagicPool = std::make_shared<MetamagicUsagePool>("rod.uses", 2);
    MetamagicGrant rod{"rod", "Test", MetamagicType::Empower};
    rod.rodGrade = MetamagicRodGrade::Lesser;
    rod.slotCost = MetamagicSlotCostPolicy::Waived;
    rod.usageCost = MetamagicUsageCost{"rod.uses", 1, metamagicPool};
    rod.isApplicable = [](const auto &, const auto &) { return true; };
    resources.addToCollection(castingMetamagicGrantsResource("prepared"), rod);
    rejects([&] { prepared.consume(slot, {std::nullopt, {{MetamagicType::Empower, "rod", {}}}}); });
    assert(metamagicPool->remaining() == 1);
    assert(prepared.slotState(slot).state == PreparedSpellSlotState::Consumed);
    spontaneous.clear(slot);
    rejects([&] { spontaneous.consume(slot, {badTargets.id}); });
    assert(spontaneous.slotState(slot) == SpontaneousSpellSlotState::Consumed);
    flexible.clearSlot(slot);
    flexible.clearPreparation(slot);
    flexible.prepare(slot, {broken.id});
    rejects([&] { flexible.consume(slot, {slot}); });
    assert(flexible.toData().consumedSlots.size() == 1);

    // Conversion exports the replacement, not the original preparation.
    prepared.clear(slot);
    prepared.prepare(slot, {broken.id});
    resources.addToCollection(spontaneousConversionsResource("prepared"), SpontaneousConversion{"conversion", "Test", [](const auto &) -> std::optional<int> { return 3; }});
    auto converted = prepared.consume(slot, {unlimited.id});
    assert(converted.maxTargets == -1 && converted.effects.size() == 3 && converted.metamagic->spellId == unlimited.id);

    GrantedSpells grants(resources, &catalog);
    resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"limited", "Test", definition.id, GrantedSpellUsage::Limited, 2});
    assert(grants.consume("limited").maxTargets == 6);
    assert(grants.toView()[0].remaining == 1);
    const auto pool = std::make_shared<UsagePool>(3);
    resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"shared", "Test", broken.id, GrantedSpellUsage::SharedPool, 0, pool, 2});
    rejects([&] { grants.consume("shared"); });
    assert(pool->remaining() == 1);
    resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"unlimited", "Test", unlimited.id, GrantedSpellUsage::AtWill});
    assert(!grants.consume("unlimited").metamagic);
    assert(grants.consume("unlimited").maxTargets == -1);
    resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"constant", "Test", definition.id, GrantedSpellUsage::Constant});
    rejects([&] { grants.consume("constant"); });
}
