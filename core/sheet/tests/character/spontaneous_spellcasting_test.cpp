#include "golarion/character/spontaneous_spellcasting.hpp"
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
    const auto slots = spellSlotsResource("sorcerer");
    const auto exemptions = consumptionExemptionsResource("sorcerer");
    const SpellSlotId third{"base", 3, 0}, fifth{"base", 5, 0};
    const SpellSlotGrant base{"base", "Class", {{2, 1}, {3, 2}, {5, 1}}};
    {
        SpontaneousSpellcasting casting(resources, "sorcerer", CharacterClassType::Sorcerer);
        resources.addToCollection(slots, base);
        resources.addToCollection(slots, SpellSlotGrant{"empty", "Test", {{3, 1}}, std::set<std::string>{}});
        resources.addToCollection(slots, SpellSlotGrant{"allowed", "Test", {{3, 1}}, std::set<std::string>{"fireball"}});
        assert(casting.toData().consumedSlots.empty());
        assert(casting.slotState(third) == SpontaneousSpellSlotState::Free);
        rejects([&] { casting.consume(third, "fireball"); });
        casting.addToRepertoire("fireball");
        rejects([&] { casting.consume({"base", 2, 0}, "fireball"); });
        rejects([&] { casting.consume({"empty", 3, 0}, "fireball"); });
        rejects([&] { casting.consume(third, " "); });
        casting.addToRepertoire("unknown");
        rejects([&] { casting.consume(third, "unknown"); });
        const auto result = casting.consume({" base ", 3, 0}, " fireball ");
        assert(result.spellId == "fireball" && result.effectiveSpellLevel == 3);
        assert(casting.slotState(third) == SpontaneousSpellSlotState::Consumed);
        rejects([&] { casting.consume(third, "fireball"); });
        casting.consume({"base", 3, 1}, "fireball");
        casting.consume({"allowed", 3, 0}, "fireball");
        casting.clear(third);
        casting.clear(third);
        casting.consume(third, "fireball"); // Consuming never removes repertoire membership.
        const auto saved = casting.toData();
        const auto view = casting.toView();
        assert(saved.consumedSlots.size() == 3);
        assert(view.groups.size() == 3 && view.spellListClass == CharacterClassType::Sorcerer);
        assert(view.groups[0].grant.id == "allowed" && view.groups[0].grant.allowedSpellIds->contains("fireball"));
        assert(view.groups[1].slots.front().state == SpontaneousSpellSlotState::Free);
        casting.load({});
        assert(casting.toData().consumedSlots.empty());
        assert(view.groups[0].slots[0].state == SpontaneousSpellSlotState::Consumed);
        casting.load(saved);
        auto bad = saved;
        bad.consumedSlots.push_back({" base ", 3, 0});
        rejects([&] { casting.load(bad); });
        bad = saved;
        bad.consumedSlots.back().groupId = "missing";
        rejects([&] { casting.load(bad); });
        assert(casting.toData().consumedSlots == saved.consumedSlots);
        for (const SpellSlotId invalid : {SpellSlotId{"missing", 3, 0}, {" ", 3, 0}, {"base", -1, 0}, {"base", 4, 0}, {"base", 3, 2}, {"base", 3, std::numeric_limits<std::size_t>::max()}})
        {
            rejects([&] { casting.consume(invalid, "fireball"); });
            rejects([&] { casting.clear(invalid); });
            rejects([&] { casting.slotState(invalid); });
            rejects([&] { casting.load({{invalid}}); });
        }
        resources.removeFromCollection(slots, "base");
        resources.addToCollection(slots, base);
        assert(casting.slotState(third) == SpontaneousSpellSlotState::Free);
        assert(casting.slotState({"allowed", 3, 0}) == SpontaneousSpellSlotState::Consumed);

        auto pool = std::make_shared<MetamagicUsagePool>("rod", 2);
        MetamagicGrant rod{"rod", "Item", MetamagicType::Empower};
        rod.slotCost = MetamagicSlotCostPolicy::Waived;
        rod.rodGrade = MetamagicRodGrade::Lesser;
        rod.usageCost = MetamagicUsageCost{"rod", 1, pool};
        rod.isApplicable = [](const auto &spell, const auto &) { return spell.id == "fireball"; };
        const MetamagicApplication application{MetamagicType::Empower, "rod", {}};
        resources.addToCollection(castingMetamagicGrantsResource("sorcerer"), rod);
        resources.addToCollection(exemptions, SpellConsumptionExemption{"throws", "Test", [](const auto &) -> bool { throw std::runtime_error("query failed"); }});
        rejects<std::runtime_error>([&] { casting.consume(third, "fireball", {application}); });
        assert(pool->remaining() == 2 && casting.slotState(third) == SpontaneousSpellSlotState::Free);
        resources.removeFromCollection(exemptions, "throws");
        resources.addToCollection(exemptions, SpellConsumptionExemption{"repeat", "Test", [](const auto &context) { return context.spell.id == "fireball"; }});
        casting.consume(third, "fireball", {application});
        casting.consume(third, "fireball", {application});
        assert(pool->remaining() == 0 && casting.slotState(third) == SpontaneousSpellSlotState::Free);
        rejects([&] { casting.consume(third, "fireball", {application}); });
        resources.removeFromCollection(exemptions, "repeat");
        auto feat = rod;
        feat.id = "feat";
        feat.rodGrade.reset();
        feat.usageCost.reset();
        feat.slotCost = MetamagicSlotCostPolicy::Standard;
        const MetamagicApplication featApplication{MetamagicType::Empower, "feat", {}};
        resources.addToCollection(preparationMetamagicGrantsResource("sorcerer"), feat);
        rejects([&] { casting.consume(fifth, "fireball", {featApplication}); });
        feat.castingScope = CastingMetamagicScope::SpontaneousConversionOnly;
        resources.addToCollection(castingMetamagicGrantsResource("sorcerer"), feat);
        rejects([&] { casting.consume(fifth, "fireball", {featApplication}); });
        resources.removeFromCollection(castingMetamagicGrantsResource("sorcerer"), "feat");
        feat.castingScope = CastingMetamagicScope::AnyCast;
        resources.addToCollection(castingMetamagicGrantsResource("sorcerer"), feat);
        rejects([&] { casting.consume(third, "fireball", {featApplication}); });
        rejects([&] { casting.consume(fifth, "fireball", {featApplication, featApplication}); });
        assert(casting.consume(fifth, "fireball", {featApplication}).requiredSlotLevel == 5);
        assert(casting.toData().consumedSlots.size() == 2); // No stored spell/metamagic selection.

        SpontaneousSpellcasting other(resources, "other", CharacterClassType::Bard);
        resources.addToCollection(spellSlotsResource("other"), base);
        other.addToRepertoire("fireball");
        rejects([&] { other.consume(third, "fireball"); });
        other.load({{third}}); // Consumption Data needs groups, not a reconstructed repertoire.
        assert(other.slotState(third) == SpontaneousSpellSlotState::Consumed);
        assert(casting.slotState(third) == SpontaneousSpellSlotState::Free);
        rejects([&] { resources.addToCollection(exemptions, SpellConsumptionExemption{"bad", "Test", {}}); });
        rejects([&] { resources.addToCollection(exemptions, 1); });
    }
    rejects([&] { resources.removeFromCollection(exemptions, "missing"); });
    resources.registerCollectionResource<int>(exemptions, [](int) {}, [](std::string_view) {});
    rejects([&] { SpontaneousSpellcasting fail(resources, "sorcerer", CharacterClassType::Sorcerer); });
    resources.addToCollection(exemptions, 1);
    resources.unregisterCollectionResource(exemptions);
    int specialLevel = 3;
    SpontaneousSpellcasting special(resources, "sorcerer", CharacterClassType::Bard,
        [&](auto, const auto &group, auto) -> std::optional<int>
        {
            if (specialLevel == 99) throw std::runtime_error("level query failed");
            return group.allowedSpellIds ? std::optional<int>{specialLevel} : std::nullopt;
        });
    resources.addToCollection(slots, SpellSlotGrant{"base", "Special", {{3, 1}}, std::set<std::string>{"fireball"}});
    special.addToRepertoire("fireball");
    special.consume(third, "fireball");
    special.clear(third);
    for (const auto level : {-1, 4, 10})
    {
        specialLevel = level;
        rejects([&] { special.consume(third, "fireball"); });
    }
    specialLevel = 99;
    rejects<std::runtime_error>([&] { special.consume(third, "fireball"); });
    assert(special.slotState(third) == SpontaneousSpellSlotState::Free);
}
