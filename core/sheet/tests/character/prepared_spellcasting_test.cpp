#include "golarion/character/prepared_spellcasting.hpp"
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
    int effectiveLevel = 2;
    PreparedSpellcasting casting(resources, "wizard", CharacterClassType::Wizard,
        [&](CharacterClassType list, const SpellSlotGrant &group, std::string_view id) -> std::optional<int>
        {
            assert(list == CharacterClassType::Wizard);
            if (id == "missing") return std::nullopt;
            if (id == "outside" && !group.allowedSpellIds) return std::nullopt;
            if (effectiveLevel == 99) throw std::runtime_error("resolver failed");
            return effectiveLevel;
        });
    const auto endpoint = spellSlotsResource("wizard");
    const SpellSlotGrant base{"base", "Class", {{1, 1}, {2, 2}, {3, 1}}};
    resources.addToCollection(endpoint, base);
    resources.addToCollection(endpoint, SpellSlotGrant{"special", "Special", {{1, 1}, {2, 1}, {3, 1}}, std::set<std::string>{"outside"}});
    resources.addToCollection(endpoint, SpellSlotGrant{"empty", "Empty", {{2, 1}}, std::set<std::string>{}});
    const SpellSlotId slot{"base", 2, 0};
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Free);
    assert(!casting.slotState(slot).spellId);
    rejects([&] { casting.prepare(slot, {"fireball"}); });
    casting.addToRepertoire("fireball");
    for (const auto *id : {"outside", "missing", "throws"}) rejects([&] { casting.addToRepertoire(id); });
    casting.addToRepertoire(" fireball ");
    rejects([&] { casting.addToRepertoire(" "); });
    rejects([&] { casting.prepare(slot, {" "}); });
    rejects([&] { casting.consume(slot); });
    casting.clear(slot);

    casting.prepare({" base ", 2, 0}, {" fireball "});
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Prepared);
    assert(casting.slotState(slot).spellId == "fireball");
    rejects([&] { casting.prepare(slot, {"outside"}); });
    assert(casting.slotState(slot).spellId == "fireball");
    casting.prepare({"base", 2, 1}, {"fireball"});
    casting.consume(slot);
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Consumed);
    assert(casting.slotState(slot).spellId == "fireball");
    assert(casting.slotState({"base", 2, 1}).state == PreparedSpellSlotState::Prepared);
    rejects([&] { casting.consume(slot); });
    rejects([&] { casting.prepare(slot, {"fireball"}); });
    casting.clear(slot);
    assert(!casting.slotState(slot).spellId);
    casting.prepare(slot, {"fireball"});
    casting.clear(slot);
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Free);
    rejects([&] { casting.prepare({"base", 1, 0}, {"fireball"}); });
    casting.prepare({"base", 3, 0}, {"fireball"});

    rejects([&] { casting.prepare(slot, {"outside"}); });
    rejects([&] { casting.prepare({"special", 2, 0}, {"fireball"}); });
    rejects([&] { casting.prepare({"empty", 2, 0}, {"fireball"}); });
    rejects([&] { casting.prepare({"special", 1, 0}, {"outside"}); });
    rejects([&] { casting.prepare({"special", 2, 0}, {"outside"}); });
    rejects([&] { casting.prepare({"special", 3, 0}, {"outside"}); });
    rejects([&] { casting.prepare(slot, {"missing"}); });
    effectiveLevel = 99;
    rejects<std::runtime_error>([&] { casting.prepare(slot, {"fireball"}); });
    for (int invalid : {-1, 10})
    {
        effectiveLevel = invalid;
        rejects([&] { casting.prepare(slot, {"fireball"}); });
        assert(casting.slotState(slot).state == PreparedSpellSlotState::Free);
    }
    effectiveLevel = 2;

    for (const SpellSlotId bad : {SpellSlotId{"unknown", 2, 0}, {" ", 2, 0}, {"base", -1, 0}, {"base", 4, 0}, {"base", 2, 2}, {"base", 2, std::numeric_limits<std::size_t>::max()}})
    {
        rejects([&] { casting.prepare(bad, {"fireball"}); });
        rejects([&] { casting.consume(bad); });
        rejects([&] { casting.clear(bad); });
        rejects([&] { casting.slotState(bad); });
    }

    casting.prepare(slot, {"fireball"});
    casting.consume(slot);
    rejects([&] { resources.addToCollection(endpoint, base); });
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Consumed);
    resources.removeFromCollection(endpoint, " base ");
    rejects([&] { casting.slotState(slot); });
    resources.addToCollection(endpoint, SpellSlotGrant{"base", "Class", {{2, 1}}});
    assert(casting.slotState(slot).state == PreparedSpellSlotState::Free);
    assert(!casting.slotState(slot).spellId);
    rejects([&] { casting.slotState({"base", 2, 1}); });
    assert(casting.slotState({"special", 2, 0}).state == PreparedSpellSlotState::Free);
    casting.prepare(slot, {"fireball"});
    resources.removeFromCollection(endpoint, "base");
    resources.addToCollection(endpoint, base);
    assert(casting.slotState({"base", 2, 1}).state == PreparedSpellSlotState::Free);
    assert(casting.slotState({"base", 3, 0}).state == PreparedSpellSlotState::Free);

    PreparedSpellcasting other(resources, "other", CharacterClassType::Bard);
    resources.addToCollection(spellSlotsResource("other"), base);
    assert(other.slotState(slot).state == PreparedSpellSlotState::Free);
    other.addToRepertoire("fireball");
    rejects([&] { other.prepare(slot, {"fireball"}); });
    assert(other.slotState(slot).state == PreparedSpellSlotState::Free);

    PreparedSpellcasting wizard(resources, "catalogWizard", CharacterClassType::Wizard);
    resources.addToCollection(spellSlotsResource("catalogWizard"), SpellSlotGrant{"base", "Class", {{2, 1}, {3, 1}, {4, 1}}});
    wizard.addToRepertoire("fireball");
    rejects([&] { wizard.prepare({"base", 2, 0}, {"fireball"}); });
    wizard.prepare({"base", 3, 0}, {"fireball"});
    wizard.prepare({"base", 4, 0}, {"fireball"});
    assert(wizard.slotState({"base", 3, 0}).spellId == "fireball");
    other.addToRepertoire("fireball");
    rejects([&] { other.prepare({"base", 3, 0}, {"fireball"}); });
    resources.addToCollection(spellSlotsResource("other"), SpellSlotGrant{"special", "Special", {{3, 1}}, std::set<std::string>{"fireball"}});
    rejects([&] { other.prepare({"special", 3, 0}, {"fireball"}); });
    assert(other.slotState({"special", 3, 0}).state == PreparedSpellSlotState::Free);

    wizard.consume({"base", 3, 0});
    const auto saved = wizard.toData();
    assert(saved.preparations.size() == 2);
    assert(saved.preparations[0].slot == (SpellSlotId{"base", 3, 0}));
    assert(saved.preparations[0].spellId == "fireball" && saved.preparations[0].consumed);
    assert(!saved.preparations[1].consumed);

    const auto view = wizard.toView();
    assert(view.spellListClass == CharacterClassType::Wizard);
    assert(view.groups.size() == 1);
    assert(view.groups[0].grant.id == "base");
    assert(view.groups[0].grant.source == "Class");
    assert(!view.groups[0].grant.allowedSpellIds);
    assert(view.groups[0].slots.size() == 3);
    assert(view.groups[0].slots[0].state == PreparedSpellSlotState::Free);
    assert(!view.groups[0].slots[0].spellId);
    assert(view.groups[0].slots[1].state == PreparedSpellSlotState::Consumed);
    assert(view.groups[0].slots[1].spellId == "fireball");
    assert(view.groups[0].slots[2].state == PreparedSpellSlotState::Prepared);

    PreparedSpellcasting restored(resources, "restored", CharacterClassType::Wizard);
    assert(restored.toView().groups.empty() && restored.toData().preparations.empty());
    rejects([&] { restored.load(saved); });
    resources.addToCollection(spellSlotsResource("restored"), SpellSlotGrant{"base", "Class", {{2, 1}, {3, 1}, {4, 1}}});
    restored.load(saved); // Historical preparation does not require current knowledge.
    restored.addToRepertoire("fireball");
    restored.load(saved);
    assert(restored.slotState({"base", 3, 0}).state == PreparedSpellSlotState::Consumed);
    assert(restored.slotState({"base", 4, 0}).state == PreparedSpellSlotState::Prepared);
    assert(restored.slotState({"base", 2, 0}).state == PreparedSpellSlotState::Free);

    const auto rejectLoad = [&](PreparedSpellcastingData bad)
    {
        rejects([&] { restored.load(bad); });
        assert(restored.toData().preparations.size() == 2);
        assert(restored.slotState({"base", 3, 0}).state == PreparedSpellSlotState::Consumed);
        assert(restored.slotState({"base", 4, 0}).spellId == "fireball");
    };
    auto bad = saved;
    bad.preparations.push_back({{" base ", 3, 0}, "fireball", false});
    rejectLoad(bad);
    bad = saved;
    bad.preparations.back().slot.groupId = "unknown";
    rejectLoad(bad);
    bad = saved;
    bad.preparations.back().slot.index = 1;
    rejectLoad(bad);
    bad = saved;
    bad.preparations.back().slot.spellLevel = 2;
    rejectLoad(bad);
    bad = saved;
    bad.preparations.back().spellId = " ";
    rejectLoad(bad);
    bad = saved;
    bad.preparations.back().spellId = "unknown";
    rejectLoad(bad);
    rejects([&] { other.load(saved); });
    assert(other.toData().preparations.empty());

    auto normalized = saved;
    normalized.preparations[0].slot.groupId = " base ";
    normalized.preparations[0].spellId = " fireball ";
    restored.load(normalized);
    assert(restored.toData().preparations[0].slot.groupId == "base");
    assert(restored.toData().preparations[0].spellId == "fireball");
    restored.load({});
    assert(restored.toData().preparations.empty());
    assert(restored.slotState({"base", 3, 0}).state == PreparedSpellSlotState::Free);
    assert(view.groups[0].slots[1].state == PreparedSpellSlotState::Consumed);
    assert(saved.preparations[0].consumed);

    const auto filteredView = casting.toView();
    assert(filteredView.groups[1].grant.id == "empty");
    assert(filteredView.groups[1].grant.allowedSpellIds->empty());
    assert(filteredView.groups[2].grant.id == "special");
    assert(filteredView.groups[2].grant.allowedSpellIds->contains("outside"));
    restored.load(saved);
    resources.removeFromCollection(spellSlotsResource("restored"), "base");
    assert(restored.toData().preparations.empty());
    assert(restored.toView().groups.empty());

    PreparedSpellcasting linked(resources, "linked", CharacterClassType::Wizard);
    const auto linkedSlots = spellSlotsResource("linked");
    const auto rules = preparationCostRulesResource("linked");
    const auto exemptions = preparationCostExemptionsResource("linked");
    resources.addToCollection(linkedSlots, SpellSlotGrant{"a", "Class", {{3, 2}, {4, 1}}});
    resources.addToCollection(linkedSlots, SpellSlotGrant{"b", "Class", {{3, 1}}});
    resources.addToCollection(linkedSlots, SpellSlotGrant{"blocked", "Class", {{3, 1}}, std::set<std::string>{}});
    linked.addToRepertoire("fireball");
    resources.addToCollection(rules, SpellPreparationCostRule{"opposition", "Wizard", SpellSchool::Evocation, 2});
    const SpellSlotId first{"a", 3, 0}, second{"b", 3, 0};
    const std::vector<SpellSlotId> pair{second, first};
    rejects([&] { linked.prepare(first, {"fireball"}); });
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{}, {"fireball"}); });
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{first, {" a ", 3, 0}}, {"fireball"}); });
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{first, {"a", 4, 0}}, {"fireball"}); });
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{first, {"blocked", 3, 0}}, {"fireball"}); });
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{first, second, {"a", 3, 1}}, {"fireball"}); });
    assert(linked.toData().preparations.empty());
    linked.prepare(pair, {"fireball"});
    assert(linked.toData().preparations.size() == 1);
    assert(linked.toData().preparations[0].additionalSlots == std::vector<SpellSlotId>{second});
    assert(linked.toView().groups[0].slots[0].preparationSlot == first);
    assert(linked.toView().groups[1].slots[0].preparationSlot == first);
    assert(!linked.toView().groups[0].slots[1].preparationSlot);
    rejects([&] { linked.prepare(std::vector<SpellSlotId>{{"a", 3, 1}, second}, {"fireball"}); });
    assert(linked.slotState({"a", 3, 1}).state == PreparedSpellSlotState::Free);
    linked.consume(second);
    assert(linked.slotState(first).state == PreparedSpellSlotState::Consumed);
    rejects([&] { linked.consume(first); });
    const auto linkedData = linked.toData();
    linked.clear(second);
    assert(linked.slotState(first).state == PreparedSpellSlotState::Free);
    linked.load(linkedData);
    assert(linked.slotState(second).state == PreparedSpellSlotState::Consumed);
    auto invalidLinkedData = linkedData;
    invalidLinkedData.preparations.push_back(linkedData.preparations.front());
    rejects([&] { linked.load(invalidLinkedData); });
    invalidLinkedData = linkedData;
    invalidLinkedData.preparations[0].additionalSlots.clear();
    rejects([&] { linked.load(invalidLinkedData); });
    assert(linked.slotState(first).state == PreparedSpellSlotState::Consumed);
    resources.removeFromCollection(linkedSlots, "b");
    assert(linked.slotState(first).state == PreparedSpellSlotState::Free);
    assert(linked.toData().preparations.empty());
    resources.addToCollection(exemptions, SpellPreparationCostExemption{"research", "Feat", "opposition"});
    linked.prepare(first, {"fireball"});
    resources.removeFromCollection(exemptions, "research");
    // Rule changes affect future preparation, not historical prepared snapshots.
    linked.consume(first);
    linked.clear(first);
    rejects([&] { linked.prepare(first, {"fireball"}); });
    resources.removeFromCollection(rules, "opposition");
    linked.prepare(first, {"fireball"});
}
