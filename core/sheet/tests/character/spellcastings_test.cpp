#include "golarion/character/spellcastings.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    static_assert(!std::is_copy_constructible_v<Spellcastings>);
    static_assert(!std::is_move_constructible_v<Spellcastings>);
    ResourceManager resources;
    const SpellcastingGrant wizard{"class.wizard", "Wizard", SpellcastingMode::Prepared, CharacterClassType::Wizard};
    const SpellSlotGrant slots{"base", "Class", {{3, 1}}};
    {
        Spellcastings castings(resources);
        assert(castings.toView().castings.empty());
        assert(CharacterSheet().toView().spellcastings.castings.empty());
        rejects([&] { castings.get("class.wizard"); });
        rejects([&] { Spellcastings duplicate(resources); });
        resources.addToCollection(Spellcastings::Resource, wizard);
        resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{"other", "Other", SpellcastingMode::Spontaneous, CharacterClassType::Wizard});
        resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{" flexible ", " Flexible ", SpellcastingMode::Flexible, CharacterClassType::Wizard});
        auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get(" class.wizard "));
        auto &spontaneous = dynamic_cast<SpontaneousSpellcasting &>(castings.get("other"));
        auto &flexible = dynamic_cast<FlexibleSpellcasting &>(castings.get("flexible"));
        assert(prepared.slotPools().empty());
        assert(flexible.toView().preparationGroups.empty());
        resources.addToCollection(spellSlotsResource(wizard.id), slots);
        resources.addToCollection(spellSlotsResource("other"), slots);
        rejects([&] { prepared.prepare({"base", 3, 0}, {"fireball"}); });
        prepared.addToRepertoire("fireball");
        prepared.prepare({"base", 3, 0}, {"fireball"});
        prepared.consume({"base", 3, 0});
        rejects([&] { spontaneous.consume({"base", 3, 0}, {"fireball"}); });
        resources.addToCollection(repertoireQuotasResource("other"), SpellRepertoireQuota{"known", "Class", {{3, 1}}});
        spontaneous.selectRepertoireSpell({"known", 3, "fireball"});
        spontaneous.consume({"base", 3, 0}, {"fireball"});

        resources.addToCollection(spellSlotsResource("flexible"), slots);
        resources.addToCollection(preparationSlotsResource("flexible"), SpellSlotGrant{"prepared", "Preparazione", {{3, 1}}});
        flexible.addToRepertoire("fireball");
        flexible.prepare({"prepared", 3, 0}, {"fireball"});
        flexible.consume({"base", 3, 0}, {{"prepared", 3, 0}});
        const auto snapshot = castings.toView();
        assert(snapshot.castings.size() == 3);
        assert(snapshot.castings[0].id == wizard.id);
        assert(snapshot.castings[0].source == wizard.source);
        assert(snapshot.castings[0].mode == SpellcastingMode::Prepared);
        assert(snapshot.castings[0].repertoire == std::vector<std::string>{"fireball"});
        const auto &preparedView = std::get<PreparedSpellcastingView>(snapshot.castings[0].state);
        assert(preparedView.spellListClass == CharacterClassType::Wizard);
        assert(preparedView.groups[0].slots[0].state == PreparedSpellSlotState::Consumed);
        assert(preparedView.groups[0].slots[0].spellId == "fireball");
        assert(snapshot.castings[1].id == "flexible" && snapshot.castings[1].source == "Flexible");
        assert(snapshot.castings[1].mode == SpellcastingMode::Flexible);
        const auto &flexibleView = std::get<FlexibleSpellcastingView>(snapshot.castings[1].state);
        assert(flexibleView.preparationGroups[0].preparations[0].spellId == "fireball");
        assert(flexibleView.slotGroups[0].slots[0].state == SpontaneousSpellSlotState::Consumed);
        assert(snapshot.castings[2].id == "other" && snapshot.castings[2].mode == SpellcastingMode::Spontaneous);
        assert(std::get<SpontaneousSpellcastingView>(snapshot.castings[2].state).groups[0].slots[0].state == SpontaneousSpellSlotState::Consumed);

        // Updating capacity keeps the same casting and repertoire, but resets this group.
        resources.removeFromCollection(spellSlotsResource(wizard.id), "base");
        resources.addToCollection(spellSlotsResource(wizard.id), SpellSlotGrant{"base", "Wizard", {{3, 2}, {4, 1}}});
        assert(&castings.get(wizard.id) == &prepared);
        assert(prepared.toData().preparations.empty());
        assert(preparedView.groups[0].slots.size() == 1);
        assert(preparedView.groups[0].slots[0].state == PreparedSpellSlotState::Consumed);
        assert(std::get<PreparedSpellcastingView>(castings.toView().castings[0].state).groups[0].slots.size() == 3);
        prepared.prepare({"base", 3, 1}, {"fireball"});
        assert(spontaneous.toData().consumedSlots.size() == 1);
        rejects([&] { resources.addToCollection(Spellcastings::Resource, wizard); });
        rejects([&] { resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{" class.wizard ", "Other", SpellcastingMode::Flexible, CharacterClassType::Wizard}); });
        assert(&castings.get(wizard.id) == &prepared);

        for (auto bad : {SpellcastingGrant{" ", "Source", SpellcastingMode::Prepared, CharacterClassType::Wizard},
                         SpellcastingGrant{"bad", " ", SpellcastingMode::Prepared, CharacterClassType::Wizard},
                         SpellcastingGrant{"bad", "Source", static_cast<SpellcastingMode>(999), CharacterClassType::Wizard},
                         SpellcastingGrant{"bad", "Source", SpellcastingMode::Prepared, CharacterClassType::Fighter}})
            rejects([&] { resources.addToCollection(Spellcastings::Resource, bad); });
        rejects([&] { resources.addToCollection(Spellcastings::Resource, slots); });
        rejects([&] { castings.get("bad"); });
        rejects([&] { castings.get(" "); });
        rejects([&] { resources.removeFromCollection(Spellcastings::Resource, "missing"); });
        resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{"bad", "Source", SpellcastingMode::Prepared, CharacterClassType::Wizard});

        // Failed nested construction preserves the existing endpoint and rolls back its own.
        resources.registerCollectionResource<int>(preparationSlotsResource("collision"), [](int) {}, [](std::string_view) {});
        const SpellcastingGrant collision{"collision", "Source", SpellcastingMode::Flexible, CharacterClassType::Wizard};
        rejects([&] { resources.addToCollection(Spellcastings::Resource, collision); });
        rejects([&] { castings.get("collision"); });
        rejects([&] { resources.addToCollection(spellSlotsResource("collision"), slots); });
        resources.addToCollection(preparationSlotsResource("collision"), 1);
        resources.unregisterCollectionResource(preparationSlotsResource("collision"));
        resources.addToCollection(Spellcastings::Resource, collision);

        resources.removeFromCollection(Spellcastings::Resource, " class.wizard ");
        assert(snapshot.castings[0].id == wizard.id && snapshot.castings[0].repertoire.size() == 1);
        for (const auto &entry : castings.toView().castings) assert(entry.id != wizard.id);
        rejects([&] { castings.get(wizard.id); });
        rejects([&] { resources.addToCollection(spellSlotsResource(wizard.id), slots); });
        resources.addToCollection(Spellcastings::Resource, wizard);
        auto &fresh = dynamic_cast<PreparedSpellcasting &>(castings.get(wizard.id));
        assert(fresh.slotPools().empty());
        resources.addToCollection(spellSlotsResource(wizard.id), slots);
        rejects([&] { fresh.prepare({"base", 3, 0}, {"fireball"}); });

        ResourceManager otherResources;
        Spellcastings other(otherResources);
        otherResources.addToCollection(Spellcastings::Resource, wizard);
        assert(other.get(wizard.id).slotPools().empty());
    }
    rejects([&] { resources.addToCollection(Spellcastings::Resource, wizard); });
    for (const auto id : {"class.wizard", "other", "flexible", "bad", "collision"})
    {
        rejects([&] { resources.addToCollection(spellSlotsResource(id), slots); });
        // Reconstructing all modes with a former ID verifies complete endpoint cleanup.
        { PreparedSpellcasting casting(resources, id, CharacterClassType::Wizard); }
        { SpontaneousSpellcasting casting(resources, id, CharacterClassType::Wizard); }
        { FlexibleSpellcasting casting(resources, id, CharacterClassType::Wizard); }
    }
    Spellcastings recreated(resources);
    resources.addToCollection(Spellcastings::Resource, wizard);
}
