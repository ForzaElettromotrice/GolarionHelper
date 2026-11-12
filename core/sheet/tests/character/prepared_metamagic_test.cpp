#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Exception = std::invalid_argument, typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const Exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    PreparedSpellcasting casting(resources, "wizard", CharacterClassType::Wizard);
    const auto prep = preparationMetamagicGrantsResource("wizard");
    const auto cast = castingMetamagicGrantsResource("wizard");
    resources.addToCollection(spellSlotsResource("wizard"), SpellSlotGrant{"base", "Class", {{3, 2}, {4, 1}, {5, 2}, {6, 1}, {8, 1}, {9, 1}}});
    casting.addToRepertoire("fireball");
    const SpellSlotId third{"base", 3, 0}, fifth{"base", 5, 0};
    const MetamagicApplication empower{MetamagicType::Empower, "empower", {}};
    const MetamagicApplication heighten{MetamagicType::Heighten, "heighten", HeightenMetamagicOptions{4}};
    const auto damageCheck = [](const SpellDefinition &spell, const MetamagicApplication &) { return spell.id == "fireball"; };
    MetamagicGrant empowered{"empower", "Feat", MetamagicType::Empower};
    rejects([&] { casting.prepare(fifth, {"fireball", {empower}}); });
    resources.addToCollection(prep, empowered);
    rejects([&] { casting.prepare(fifth, {"fireball", {empower}}); }); // No damage schema; provider must confirm applicability.
    resources.removeFromCollection(prep, "empower");
    empowered.isApplicable = damageCheck;
    resources.addToCollection(prep, empowered);
    resources.addToCollection(prep, MetamagicGrant{"heighten", "Feat", MetamagicType::Heighten});
    resources.addToCollection(prep, MetamagicGrant{"quicken", "Feat", MetamagicType::Quicken});
    rejects([&] { casting.prepare(third, {"fireball", {empower}}); });
    rejects([&] { casting.prepare(fifth, {"fireball", {empower, empower}}); });
    casting.prepare(fifth, {"fireball", {empower}});
    const auto saved = casting.toData();
    assert(saved.preparations.front().metamagic.size() == 1);
    assert(casting.slotState(fifth).metamagic.front().type == MetamagicType::Empower);
    assert(casting.toView().groups[0].slots[3].metamagic.size() == 1);
    const auto normal = casting.consume(fifth);
    assert(normal.requiredSlotLevel == 5 && normal.effectiveSpellLevel == 3);
    casting.clear(fifth);
    casting.load(saved);
    assert(casting.slotState(fifth).state == PreparedSpellSlotState::Prepared);
    auto bad = saved;
    bad.preparations.front().metamagic.push_back(empower);
    rejects([&] { casting.load(bad); });
    assert(casting.slotState(fifth).state == PreparedSpellSlotState::Prepared);
    casting.clear(fifth);
    const SpellSlotId eighth{"base", 8, 0};
    casting.prepare(eighth, {"fireball", {heighten, {MetamagicType::Quicken, "quicken", {}}}});
    const auto heightened = casting.consume(eighth);
    assert(heightened.requiredSlotLevel == 8 && heightened.effectiveSpellLevel == 4);
    casting.clear(eighth);

    auto pool = std::make_shared<MetamagicUsagePool>("rod.uses", 3);
    MetamagicGrant rod{"rod", "Item", MetamagicType::Empower};
    rod.rodGrade = MetamagicRodGrade::Lesser;
    rod.slotCost = MetamagicSlotCostPolicy::Waived;
    rod.usageCost = MetamagicUsageCost{"rod.uses", 1, pool};
    rod.isApplicable = damageCheck;
    resources.addToCollection(cast, rod);
    const MetamagicApplication rodEmpower{MetamagicType::Empower, "rod", {}};
    casting.prepare(third, {"fireball"});
    const auto empoweredCast = casting.consume(third, {std::nullopt, {rodEmpower}});
    assert(empoweredCast.requiredSlotLevel == 3 && empoweredCast.nominalSlotLevel == 5);
    assert(empoweredCast.applications.size() == 1 && pool->remaining() == 2);
    assert(casting.slotState(third).metamagic.empty()); // Casting additions are transient.
    casting.clear(third);
    casting.prepare(fifth, {"fireball", {empower}});
    rejects([&] { casting.consume(fifth, {std::nullopt, {rodEmpower}}); });
    assert(pool->remaining() == 2 && casting.slotState(fifth).state == PreparedSpellSlotState::Prepared);

    auto rodMax = rod;
    rodMax.id = "max-rod";
    rodMax.type = MetamagicType::Maximize;
    resources.addToCollection(cast, rodMax);
    const MetamagicApplication maximize{MetamagicType::Maximize, "max-rod", {}};
    rejects([&] { casting.consume(fifth, {std::nullopt, {maximize}}); }); // Lesser rod cannot modify a fifth-level payment.
    assert(pool->remaining() == 2);
    resources.removeFromCollection(cast, "max-rod");
    rodMax.rodGrade = MetamagicRodGrade::Normal;
    resources.addToCollection(cast, rodMax);
    const auto combined = casting.consume(fifth, {std::nullopt, {maximize}});
    assert(combined.applications.size() == 2 && combined.requiredSlotLevel == 5 && combined.nominalSlotLevel == 8);
    assert(pool->remaining() == 1);
    casting.clear(fifth);
    casting.prepare(third, {"fireball"});
    rejects([&] { casting.consume(third, {std::nullopt, {rodEmpower, maximize}}); });
    assert(pool->remaining() == 1);
    resources.addToCollection(consumptionExemptionsResource("wizard"), SpellConsumptionExemption{"repeat", "Test", [](const auto &) { return true; }});
    casting.consume(third, {std::nullopt, {rodEmpower}});
    assert(pool->remaining() == 0 && casting.slotState(third).state == PreparedSpellSlotState::Prepared);
    rejects([&] { casting.consume(third, {std::nullopt, {rodEmpower}}); });
    assert(casting.slotState(third).state == PreparedSpellSlotState::Prepared);
    resources.removeFromCollection(cast, "rod");
    resources.addToCollection(cast, rod);
    assert(pool->remaining() == 0); // Grant lifecycle does not refill a source.
    PreparedSpellcasting other(resources, "other", CharacterClassType::Wizard);
    resources.addToCollection(spellSlotsResource("other"), SpellSlotGrant{"base", "Class", {{3, 1}}});
    resources.addToCollection(castingMetamagicGrantsResource("other"), rod);
    other.addToRepertoire("fireball");
    other.prepare(third, {"fireball"});
    rejects([&] { other.consume(third, {std::nullopt, {rodEmpower}}); });
    pool->reset(1);
    other.consume(third, {std::nullopt, {rodEmpower}});
    assert(pool->remaining() == 0);
    resources.removeFromCollection(consumptionExemptionsResource("wizard"), "repeat");
    casting.clear(third);

    // A source used during preparation is charged once, not again on load/casting.
    auto preparationPool = std::make_shared<MetamagicUsagePool>("preparation.uses", 1);
    MetamagicGrant silent{"silent", "Feature", MetamagicType::Silent};
    silent.usageCost = MetamagicUsageCost{"preparation.uses", 1, preparationPool};
    resources.addToCollection(prep, silent);
    const SpellSlotId fourth{"base", 4, 0};
    casting.prepare(fourth, {"fireball", {{MetamagicType::Silent, "silent", {}}}});
    assert(preparationPool->remaining() == 0);
    const auto limitedSaved = casting.toData();
    casting.load(limitedSaved);
    resources.removeFromCollection(prep, "silent");
    casting.consume(fourth); // Already-prepared metamagic is a snapshot.
    assert(preparationPool->remaining() == 0);
    casting.clear(fourth);

    // All source payments are validated before any pool or slot changes.
    auto firstPool = std::make_shared<MetamagicUsagePool>("first", 2);
    auto emptyPool = std::make_shared<MetamagicUsagePool>("empty", 0);
    MetamagicGrant silentCast{"silent", "Feature", MetamagicType::Silent};
    silentCast.usageCost = MetamagicUsageCost{"first", 1, firstPool};
    MetamagicGrant stillCast{"still", "Feature", MetamagicType::Still};
    stillCast.usageCost = MetamagicUsageCost{"empty", 1, emptyPool};
    resources.addToCollection(cast, silentCast);
    resources.addToCollection(cast, stillCast);
    casting.prepare(fifth, {"fireball"});
    rejects([&] { casting.consume(fifth, {std::nullopt, {{MetamagicType::Silent, "silent", {}}, {MetamagicType::Still, "still", {}}}}); });
    assert(firstPool->remaining() == 2 && casting.slotState(fifth).state == PreparedSpellSlotState::Prepared);
    resources.removeFromCollection(cast, "still");
    stillCast.usageCost = MetamagicUsageCost{"first", 1, firstPool};
    stillCast.isApplicable = [](const auto &, const auto &) -> bool { throw std::runtime_error("applicability failed"); };
    resources.addToCollection(cast, stillCast);
    rejects<std::runtime_error>([&] { casting.consume(fifth, {std::nullopt, {{MetamagicType::Silent, "silent", {}}, {MetamagicType::Still, "still", {}}}}); });
    assert(firstPool->remaining() == 2 && casting.slotState(fifth).state == PreparedSpellSlotState::Prepared);
    rejects([&] { MetamagicRegistry::pay({{"first", 2, firstPool}, {"first", 1, firstPool}}); });
    assert(firstPool->remaining() == 2);
    rejects([&] { MetamagicRegistry::pay({{"first", 1, {}}}); });
    resources.removeFromCollection(cast, "still");
    stillCast.isApplicable = {};
    resources.addToCollection(cast, stillCast);
    casting.consume(fifth, {std::nullopt, {{MetamagicType::Silent, "silent", {}}, {MetamagicType::Still, "still", {}}}});
    assert(firstPool->remaining() == 0);
    casting.clear(fifth);

    // Conversion evaluates its own metamagic, not that of the sacrificed preparation.
    auto conversionGrant = empowered;
    conversionGrant.castingScope = CastingMetamagicScope::SpontaneousConversionOnly;
    resources.addToCollection(cast, conversionGrant);
    resources.addToCollection(spontaneousConversionsResource("wizard"), SpontaneousConversion{"convert", "Test", [](const auto &) -> std::optional<int> { return 3; }});
    casting.prepare(fifth, {"fireball", {empower}});
    rejects([&] { casting.consume(fifth, {std::nullopt, {empower}}); });
    const auto converted = casting.consume(fifth, {"fireball", {empower}});
    assert(converted.applications.size() == 1 && converted.requiredSlotLevel == 5);
    casting.clear(fifth);
    casting.prepare(third, {"fireball"});
    rejects([&] { casting.consume(third, {"fireball", {empower}}); });
    assert(casting.slotState(third).state == PreparedSpellSlotState::Prepared);
    casting.clear(third);

    // Applicability checks reject incompatible structural metadata.
    MetamagicRegistry isolated(resources, "evaluation");
    const auto endpoint = preparationMetamagicGrantsResource("evaluation");
    auto spell = SpellDefinitionManager::instance().get("fireball");
    for (const auto type : {MetamagicType::Extend, MetamagicType::Reach, MetamagicType::Selective, MetamagicType::Quicken})
        resources.addToCollection(endpoint, MetamagicGrant{std::to_string(static_cast<int>(type)), "Test", type});
    const auto application = [](MetamagicType type, MetamagicOptions options = {}) { return MetamagicApplication{type, std::to_string(static_cast<int>(type)), options}; };
    rejects([&] { isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Extend)}); });
    rejects([&] { isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Reach, ReachMetamagicOptions{1})}); });
    spell.area.reset();
    rejects([&] { isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Selective)}); });
    spell.castingTimeCategory = SpellCastingTimeCategory::Minute;
    rejects([&] { isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Quicken)}); });
    spell.castingTimeCategory = SpellCastingTimeCategory::Round;
    rejects([&] { isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Quicken)}); });
    resources.removeFromCollection(endpoint, std::to_string(static_cast<int>(MetamagicType::Quicken)));
    MetamagicGrant oneRound{std::to_string(static_cast<int>(MetamagicType::Quicken)), "Test", MetamagicType::Quicken};
    oneRound.isApplicable = [](const auto &, const auto &) { return true; }; // Provider confirms exactly one round.
    resources.addToCollection(endpoint, oneRound);
    assert(isolated.evaluate(MetamagicPhase::Preparation, spell, 3, {application(MetamagicType::Quicken)}).requiredSlotLevel == 7);
}
