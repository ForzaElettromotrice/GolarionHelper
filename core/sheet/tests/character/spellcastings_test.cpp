#include "golarion/character/spellcastings.hpp"
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
        rejects([&] { prepared.prepare({"base", 3, 0}, "fireball"); });
        prepared.addToRepertoire("fireball");
        prepared.prepare({"base", 3, 0}, "fireball");
        prepared.consume({"base", 3, 0});
        rejects([&] { spontaneous.consume({"base", 3, 0}, "fireball"); });
        spontaneous.addToRepertoire("fireball");
        spontaneous.consume({"base", 3, 0}, "fireball");

        // Updating capacity keeps the same casting and repertoire, but resets this group.
        resources.removeFromCollection(spellSlotsResource(wizard.id), "base");
        resources.addToCollection(spellSlotsResource(wizard.id), SpellSlotGrant{"base", "Wizard", {{3, 2}, {4, 1}}});
        assert(&castings.get(wizard.id) == &prepared);
        assert(prepared.toData().preparations.empty());
        prepared.prepare({"base", 3, 1}, "fireball");
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
        rejects([&] { castings.get(wizard.id); });
        rejects([&] { resources.addToCollection(spellSlotsResource(wizard.id), slots); });
        resources.addToCollection(Spellcastings::Resource, wizard);
        auto &fresh = dynamic_cast<PreparedSpellcasting &>(castings.get(wizard.id));
        assert(fresh.slotPools().empty());
        resources.addToCollection(spellSlotsResource(wizard.id), slots);
        rejects([&] { fresh.prepare({"base", 3, 0}, "fireball"); });

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
