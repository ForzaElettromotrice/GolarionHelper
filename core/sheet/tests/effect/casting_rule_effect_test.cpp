#include "golarion/character/spellcastings.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/feat_prerequisite_registry.hpp"
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
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    int permitted = 0;
    resources.registerTarget("permitted", [&] { return permitted; });
    resources.registerTarget("bab", [] { return 1; });
    resources.registerTarget("level", [] { return 8; });
    Spellcastings castings(resources);
    auto pool = std::make_shared<UsagePool>(3);
    const auto apply = [&](const char *json, std::string id)
    {
        return compileEffect(effectDefinitionFromJson(nlohmann::json::parse(json)), {id, "Fonte", [pool](std::string_view name)
        { if (name != "charges") throw std::invalid_argument("unknown pool"); return pool; }})(resources);
    };
    auto caster = apply(R"({"id":"x","type":"spellcastingGrant","mode":"prepared","spellListClass":"wizard"})", "caster");
    auto slots = apply(R"({"id":"x","type":"spellSlotGrant","castingId":"caster","pool":"casting","slots":[{"level":3,"capacity":2},{"level":4,"capacity":2}]})", "slots");
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("caster"));
    prepared.addToRepertoire("fireball");
    auto rule = apply(R"({"id":"x","type":"preparationCost","castingId":"caster","school":"evocation","requiredSlots":2})", "cost");
    rejects([&] { prepared.prepare(SpellSlotId{"slots", 3, 0}, "fireball"); });
    auto exemption = apply(R"({"id":"x","type":"preparationCostExemption","castingId":"caster","ruleId":"cost"})", "exempt");
    auto check = apply(R"({"id":"x","type":"preparationCheck","castingId":"caster","expression":"@permitted","filter":{"schools":["evocation"]}})", "check");
    rejects([&] { prepared.prepare(SpellSlotId{"slots", 3, 0}, "fireball"); });
    auto checkExempt = apply(R"({"id":"x","type":"preparationCheckExemption","castingId":"caster","checkId":"check","filter":{"spellIds":["fireball"]}})", "checkExempt");
    prepared.prepare(SpellSlotId{"slots", 3, 0}, "fireball");
    checkExempt();
    permitted = 1;
    auto preserved = apply(R"({"id":"x","type":"consumptionExemption","castingId":"caster","filter":{"spellIds":["fireball"]}})", "preserve");
    prepared.consume({"slots", 3, 0});
    prepared.consume({"slots", 3, 0});
    preserved();
    prepared.consume({"slots", 3, 0});
    rejects([&] { prepared.consume({"slots", 3, 0}); });
    prepared.clear({"slots", 3, 0});
    auto conversion = apply(R"({"id":"x","type":"spontaneousConversion","castingId":"caster","groupIds":["slots"],"replacements":[{"spellId":"fireball","level":3}]})", "conversion");
    auto rod = apply(R"({"id":"x","type":"metamagicGrant","castingId":"caster","phase":"casting","metamagic":"silent","slotCost":"waived","rodGrade":"lesser","poolId":"charges"})", "rod");
    prepared.prepare(SpellSlotId{"slots", 3, 0}, "fireball");
    prepared.consume({"slots", 3, 0}, "fireball", {{MetamagicType::Silent, "rod", {}}});
    assert(pool->remaining() == 2);
    assert(castings.toData().metamagicUses->empty());
    rod(); conversion();
    prepared.clear({"slots", 3, 0});
    auto level = apply(R"({"id":"x","type":"spellLevelOverride","castingId":"caster","groupId":"slots","spellId":"fireball","level":4})", "level");
    rejects([&] { prepared.prepare(SpellSlotId{"slots", 3, 0}, "fireball"); });
    prepared.prepare(SpellSlotId{"slots", 4, 0}, "fireball");
    prepared.clear({"slots", 4, 0});
    level(); check(); exemption(); rule(); slots(); caster();
    assert(castings.toView().castings.empty());

    auto flexibleGrant = apply(R"({"id":"x","type":"spellcastingGrant","mode":"flexible","spellListClass":"wizard"})", "flex");
    auto flexibleSlots = apply(R"({"id":"x","type":"spellSlotGrant","castingId":"flex","pool":"casting","slots":[{"level":4,"capacity":1}]})", "slots");
    auto preparationSlots = apply(R"({"id":"x","type":"spellSlotGrant","castingId":"flex","pool":"preparation","slots":[{"level":4,"capacity":1}]})", "preparations");
    auto silent = apply(R"({"id":"x","type":"metamagicGrant","castingId":"flex","phase":"preparation","metamagic":"silent"})", "silent");
    auto still = apply(R"({"id":"x","type":"metamagicGrant","castingId":"flex","phase":"casting","metamagic":"still","slotCost":"waived","poolId":"charges"})", "still");
    auto mixing = apply(R"({"id":"x","type":"metamagicMixingPermission","castingId":"flex","poolId":"charges","filter":{"spellIds":["fireball"]}})", "mix");
    auto &flex = dynamic_cast<FlexibleSpellcasting &>(castings.get("flex"));
    flex.addToRepertoire("fireball");
    flex.prepare({"preparations", 4, 0}, "fireball", {{MetamagicType::Silent, "silent", {}}});
    pool->consume();
    rejects([&] { flex.consume({"slots", 4, 0}, {"preparations", 4, 0}, {{MetamagicType::Still, "still", {}}}, "mix"); });
    assert(pool->remaining() == 1 && flex.toData().consumedSlots.empty());
    pool->refill();
    flex.consume({"slots", 4, 0}, {"preparations", 4, 0}, {{MetamagicType::Still, "still", {}}}, "mix");
    assert(pool->remaining() == 1);
    mixing(); still(); silent(); preparationSlots(); flexibleSlots(); flexibleGrant();

    FeatPrerequisiteRegistry prerequisites(resources);
    FeatDefinition feat{.id = "test", .name = "Test", .description = "Test", .types = {FeatType::Combat}};
    auto alternative = apply(R"({"id":"x","type":"featPrerequisiteAlternative","target":"bab","expression":"@level - 3","filter":{"kind":"type","featType":"combat"}})", "alternative");
    FeatPrerequisiteContext query{feat, "slot", "Fonte"};
    assert(prerequisites.values("bab", query).size() == 2);
    assert(prerequisites.values("bab", query)[1].value == 5);
    alternative();
    assert(prerequisites.values("bab", query).size() == 1);
    rejects([&] { apply(R"({"id":"x","type":"metamagicGrant","castingId":"missing","phase":"unknown","metamagic":"silent"})", "bad"); });
    rejects([&] { apply(R"({"id":"x","type":"preparationCost","castingId":"missing","school":"evocation","requiredSlots":1.5})", "bad"); });
}
