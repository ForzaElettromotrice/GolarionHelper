#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <limits>
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
    const auto slots = spellSlotsResource("arcanist");
    const auto preparations = preparationSlotsResource("arcanist");
    const auto mixing = metamagicMixingPermissionsResource("arcanist");
    const auto exemptions = consumptionExemptionsResource("arcanist");
    const SpellSlotGrant castingGrant{"base", "Class", {{2, 1}, {3, 2}, {5, 2}, {6, 1}}};
    const SpellSlotGrant preparationGrant{"base", "Class", {{2, 1}, {3, 2}, {5, 1}}};
    const SpellSlotId plain{"base", 3, 0}, second{"base", 3, 1}, enhanced{"base", 5, 0}, sixth{"base", 6, 0};
    {
        FlexibleSpellcasting casting(resources, "arcanist", CharacterClassType::Arcanist);
        resources.addToCollection(slots, castingGrant);
        resources.addToCollection(preparations, preparationGrant);
        rejects([&] { casting.consume(plain, {plain}); });
        rejects([&] { casting.prepare(plain, {"fireball"}); });
        casting.addToRepertoire("fireball");
        rejects([&] { casting.prepare({"base", 2, 0}, {"fireball"}); });
        casting.prepare({" base ", 3, 0}, {" fireball "});
        rejects([&] { casting.prepare(plain, {"fireball"}); });
        casting.consume(plain, {plain});
        casting.consume(second, {plain}); // One prepared selection, two independent payments.
        assert(casting.toData().preparations.size() == 1);
        assert(casting.toData().consumedSlots.size() == 2);
        rejects([&] { casting.consume(plain, {plain}); });
        casting.clearPreparation(plain);
        assert(casting.toData().consumedSlots.size() == 2);
        casting.clearSlot(plain);
        rejects([&] { casting.consume(plain, {plain}); });
        casting.prepare(plain, {"fireball"});
        casting.clearSlot(second);
        casting.clearSlot(second);
        assert(casting.toData().preparations.size() == 1);

        MetamagicGrant empower{"empower", "Feat", MetamagicType::Empower};
        empower.isApplicable = [](const auto &spell, const auto &) { return spell.id == "fireball"; };
        const MetamagicApplication empowerment{MetamagicType::Empower, "empower", {}};
        resources.addToCollection(preparationMetamagicGrantsResource("arcanist"), empower);
        resources.addToCollection(castingMetamagicGrantsResource("arcanist"), empower);
        casting.prepare(enhanced, {"fireball", {empowerment}});
        assert(casting.toData().preparations.size() == 2); // Same spell, distinct versions.
        rejects([&] { casting.consume(plain, {enhanced}); });
        assert(casting.consume(enhanced, {enhanced}).requiredSlotLevel == 5);
        assert(casting.toData().preparations.size() == 2);
        casting.clearSlot(enhanced);
        assert(casting.consume(enhanced, {plain, {empowerment}}).applications.size() == 1);
        casting.clearSlot(enhanced);

        auto spellPool = std::make_shared<MetamagicUsagePool>("spell-uses", 2);
        MetamagicGrant silent{"silent", "Feature", MetamagicType::Silent};
        silent.usageCost = MetamagicUsageCost{"spell-uses", 1, spellPool};
        resources.addToCollection(castingMetamagicGrantsResource("arcanist"), silent);
        const MetamagicApplication silence{MetamagicType::Silent, "silent", {}};
        rejects([&] { casting.consume(sixth, {enhanced, {silence}}); });
        assert(spellPool->remaining() == 2);
        resources.addToCollection(mixing, MetamagicMixingPermission{"no", "Test", [](const auto &) { return false; }});
        rejects([&] { casting.consume(sixth, {enhanced, {silence}}); });
        resources.addToCollection(mixing, MetamagicMixingPermission{"free", "Test", [](const auto &context) { return context.additions.size() == 1; }});
        assert(casting.consume(sixth, {enhanced, {silence}}).applications.size() == 2);
        assert(spellPool->remaining() == 1);
        casting.clearSlot(sixth);
        resources.removeFromCollection(mixing, "free");
        auto mixPool = std::make_shared<MetamagicUsagePool>("reservoir", 0);
        resources.addToCollection(mixing, MetamagicMixingPermission{"paid", "Exploit", [](const auto &) { return true; }, MetamagicUsageCost{"reservoir", 1, mixPool}});
        rejects([&] { casting.consume(sixth, {enhanced, {silence}}); }); // No implicit choice of a paid source.
        rejects([&] { casting.consume(sixth, {enhanced, {silence}, "paid"}); });
        assert(spellPool->remaining() == 1 && casting.toData().consumedSlots.empty());
        mixPool->reset(1);
        resources.addToCollection(exemptions, SpellConsumptionExemption{"throws", "Test", [](const auto &) -> bool { throw std::runtime_error("query failed"); }});
        rejects<std::runtime_error>([&] { casting.consume(sixth, {enhanced, {silence}, "paid"}); });
        assert(mixPool->remaining() == 1 && spellPool->remaining() == 1);
        resources.removeFromCollection(exemptions, "throws");
        casting.consume(sixth, {enhanced, {silence}, " paid "});
        assert(mixPool->remaining() == 0 && spellPool->remaining() == 0);
        casting.clearSlot(sixth);
        rejects([&] { casting.consume(sixth, {enhanced, {empowerment}, "paid"}); }); // Mixing never bypasses duplicate-type rules.
        rejects([&] { casting.consume(plain, {plain, {}, "paid"}); });

        const auto saved = casting.toData();
        const auto view = casting.toView();
        assert(view.preparationGroups.size() == 1 && view.slotGroups.size() == 1);
        assert(!view.preparationGroups[0].preparations[0].spellId);
        assert(view.preparationGroups[0].preparations.back().metamagic.size() == 1);
        casting.consume(plain, {plain});
        const auto consumedData = casting.toData();
        casting.load({});
        assert(casting.toData().preparations.empty() && casting.toData().consumedSlots.empty());
        casting.load(consumedData);
        assert(casting.toData().consumedSlots.size() == 1);
        assert(view.slotGroups[0].slots[1].state == SpontaneousSpellSlotState::Free);
        auto bad = consumedData;
        bad.preparations.push_back(bad.preparations.front());
        rejects([&] { casting.load(bad); });
        bad = consumedData;
        bad.consumedSlots.push_back({" base ", 3, 0});
        rejects([&] { casting.load(bad); });
        bad = consumedData;
        bad.preparations.back().metamagic.push_back(empowerment);
        rejects([&] { casting.load(bad); });
        bad = consumedData;
        bad.consumedSlots.push_back({"unknown", 3, 0});
        rejects([&] { casting.load(bad); });
        assert(casting.toData().consumedSlots == consumedData.consumedSlots);
        assert(casting.toData().preparations.size() == 2);
        assert(spellPool->remaining() == 0 && mixPool->remaining() == 0);

        resources.removeFromCollection(slots, "base");
        assert(casting.toData().preparations.size() == 2 && casting.toData().consumedSlots.empty());
        resources.addToCollection(slots, castingGrant);
        casting.consume(plain, {plain});
        resources.removeFromCollection(preparations, "base");
        assert(casting.toData().preparations.empty() && casting.toData().consumedSlots.size() == 1);
        resources.addToCollection(preparations, preparationGrant);
        casting.load(saved);
        resources.addToCollection(preparations, SpellSlotGrant{"blocked", "Test", {{3, 1}}, std::set<std::string>{}});
        resources.addToCollection(slots, SpellSlotGrant{"blocked", "Test", {{3, 1}}, std::set<std::string>{}});
        rejects([&] { casting.prepare({"blocked", 3, 0}, {"fireball"}); });
        rejects([&] { casting.consume({"blocked", 3, 0}, {plain}); });
        for (const SpellSlotId invalid : {SpellSlotId{"missing", 3, 0}, {" ", 3, 0}, {"base", -1, 0}, {"base", 3, std::numeric_limits<std::size_t>::max()}})
        {
            rejects([&] { casting.prepare(invalid, {"fireball"}); });
            rejects([&] { casting.clearPreparation(invalid); });
            rejects([&] { casting.clearSlot(invalid); });
            rejects([&] { casting.consume(invalid, {plain}); });
            rejects([&] { casting.consume(plain, {invalid}); });
        }
        rejects([&] { resources.addToCollection(preparations, preparationGrant); });
        rejects([&] { resources.addToCollection(preparations, SpellSlotGrant{"bad", "Test", {{3, 0}}}); });
        rejects([&] { resources.addToCollection(mixing, MetamagicMixingPermission{"bad", "Test", {}}); });
        rejects([&] { resources.addToCollection(mixing, MetamagicMixingPermission{"bad", "Test", [](const auto &) { return true; }, MetamagicUsageCost{"pool", 0}}); });
        rejects([&] { resources.addToCollection(exemptions, SpellConsumptionExemption{"bad", "Test", {}}); });
        resources.addToCollection(exemptions, SpellConsumptionExemption{"atWill", "Test", [](const auto &) { return true; }});
        casting.consume(plain, {plain});
        casting.consume(plain, {plain});
        assert(casting.toData().consumedSlots.empty());
        FlexibleSpellcasting other(resources, "other", CharacterClassType::Arcanist);
        resources.addToCollection(spellSlotsResource("other"), castingGrant);
        resources.addToCollection(preparationSlotsResource("other"), preparationGrant);
        rejects([&] { other.consume(plain, {plain}); });
    }
    for (const auto &endpoint : {preparations, mixing, exemptions})
    {
        rejects([&] { resources.removeFromCollection(endpoint, "anything"); });
        resources.registerCollectionResource<int>(endpoint, [](int) {}, [](std::string_view) {});
        rejects([&] { FlexibleSpellcasting fail(resources, "arcanist", CharacterClassType::Arcanist); });
        resources.addToCollection(endpoint, 1);
        resources.unregisterCollectionResource(endpoint);
        FlexibleSpellcasting retry(resources, "arcanist", CharacterClassType::Arcanist);
    }
}
