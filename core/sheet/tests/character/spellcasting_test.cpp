#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace
{
    template<typename Casting> void checkConstruction(golarion::CharacterClassType list)
    {
        static_assert(std::is_base_of_v<golarion::Spellcasting, Casting>);
        static_assert(!std::is_abstract_v<Casting>);
        static_assert(!std::is_default_constructible_v<Casting>);
        static_assert(!std::is_convertible_v<golarion::CharacterClassType, Casting>);
        static_assert(!std::is_constructible_v<Casting, int>);
        static_assert(!std::is_copy_constructible_v<Casting>);
        static_assert(!std::is_move_constructible_v<Casting>);
        golarion::ResourceManager resources;
        std::unique_ptr<golarion::Spellcasting> casting = std::make_unique<Casting>(resources, "test", list);
        assert(dynamic_cast<Casting *>(casting.get()) != nullptr);
    }

    template<typename Casting> void rejectsInvalidList(int invalid)
    {
        bool rejected = false;
        golarion::ResourceManager resources;
        try { const Casting casting(resources, "test", static_cast<golarion::CharacterClassType>(invalid)); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }

    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }

    template<typename Casting> void checkSlots()
    {
        using namespace golarion;
        ResourceManager resources;
        const auto endpoint = spellSlotsResource("test");
        const SpellSlotGrant base{"base", "Class", {{1, 3}, {2, 2}}};
        {
            Casting casting(resources, " test ", CharacterClassType::Cleric);
            Casting other(resources, "other", CharacterClassType::Bard);
            assert(casting.slotPools().empty());
            resources.addToCollection(endpoint, base);
            resources.addToCollection(spellSlotsResource("other"), base);
            assert(!casting.slotPools().at("base").allowedSpellIds);
            assert(casting.slotPools().at("base").dailyCapacityByLevel == base.dailyCapacityByLevel);
            resources.addToCollection(endpoint, SpellSlotGrant{" domain ", " Domain ", {{1, 1}, {2, 1}}, std::set<std::string>{" fireball "}});
            assert(casting.slotPools().at("domain").source == "Domain");
            assert(casting.slotPools().at("domain").allowedSpellIds->contains("fireball"));
            assert(casting.slotPools().at("domain").dailyCapacityByLevel.size() == 2);
            resources.addToCollection(endpoint, SpellSlotGrant{"empty", "Spirit", {{1, 1}}, std::set<std::string>{}});
            assert(casting.slotPools().at("empty").allowedSpellIds.has_value());
            assert(casting.slotPools().at("empty").allowedSpellIds->empty());
            rejects([&] { resources.addToCollection(endpoint, base); });
            rejects([&] { resources.addToCollection(endpoint, 1); });
            rejects([&] { Casting duplicate(resources, "test", CharacterClassType::Druid); });
            rejects([&] { Casting blank(resources, " ", CharacterClassType::Druid); });
            for (int level : {-1, 10})
                rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", "Class", {{1, 1}, {level, 1}}}); });
            for (int count : {-1, 0})
                rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", "Class", {{1, 1}, {2, count}}}); });
            rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", "Class", {}}); });
            rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{" ", "Class", {{1, 1}}}); });
            rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", " ", {{1, 1}}}); });
            rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", "Class", {{1, 1}}, std::set<std::string>{" "}}); });
            rejects([&] { resources.addToCollection(endpoint, SpellSlotGrant{"bad", "Class", {{1, 1}}, std::set<std::string>{"x", " x "}}); });
            assert(casting.slotPools().size() == 3);
            resources.removeFromCollection(endpoint, " base ");
            assert(!casting.slotPools().contains("base"));
            resources.addToCollection(endpoint, SpellSlotGrant{"base", "Class", {{1, 4}, {3, 1}}});
            assert(casting.slotPools().at("base").dailyCapacityByLevel.at(1) == 4);
            assert(!casting.slotPools().at("base").dailyCapacityByLevel.contains(2));
            assert(casting.slotPools().at("base").dailyCapacityByLevel.at(3) == 1);
            assert(other.slotPools().at("base").dailyCapacityByLevel == base.dailyCapacityByLevel);
            assert(casting.slotPools().at("domain").dailyCapacityByLevel.at(1) == 1);
            assert(casting.slotPools().at("domain").dailyCapacityByLevel.at(2) == 1);
            resources.removeFromCollection(endpoint, "empty");
            rejects([&] { resources.removeFromCollection(endpoint, "empty"); });
            resources.addToCollection(endpoint, SpellSlotGrant{"cantrips", "Class", {{0, 3}}});
            resources.addToCollection(endpoint, SpellSlotGrant{"highest", "Class", {{9, 1}}});
        }
        rejects([&] { resources.addToCollection(endpoint, base); });
        Casting recreated(resources, "test", CharacterClassType::Druid);
        assert(recreated.slotPools().empty());
        resources.addToCollection(endpoint, base);
        assert(recreated.slotPools().size() == 1);
    }
}

int main()
{
    using namespace golarion;
    checkSlots<PreparedSpellcasting>();
    checkSlots<SpontaneousSpellcasting>();
    checkSlots<FlexibleSpellcasting>();
    static_assert(std::is_enum_v<CharacterClassType>);
    static_assert(std::is_abstract_v<Spellcasting>);
    static_assert(std::has_virtual_destructor_v<Spellcasting>);
    static_assert(!std::is_constructible_v<Spellcasting, CharacterClassType>);
    static_assert(!std::is_constructible_v<Spellcasting, CharacterClassType, int>);
    static_assert(!std::is_default_constructible_v<Spellcasting>);
    static_assert(!std::is_convertible_v<CharacterClassType, Spellcasting>);
    static_assert(!std::is_constructible_v<Spellcasting, int>);

    for (const CharacterClassType list : {
        CharacterClassType::Adept,
        CharacterClassType::Alchemist,
        CharacterClassType::Antipaladin,
        CharacterClassType::Arcanist,
        CharacterClassType::Sorcerer,
        CharacterClassType::Skald,
        CharacterClassType::Oracle,
        CharacterClassType::Warpriest,
        CharacterClassType::Investigator,
        CharacterClassType::Wizard,
        CharacterClassType::Bard,
        CharacterClassType::Hunter,
        CharacterClassType::Cleric,
        CharacterClassType::Summoner,
        CharacterClassType::UnchainedSummoner,
        CharacterClassType::Druid,
        CharacterClassType::Witch,
        CharacterClassType::Inquisitor,
        CharacterClassType::Bloodrager,
        CharacterClassType::Magus,
        CharacterClassType::Medium,
        CharacterClassType::Mesmerist,
        CharacterClassType::Occultist,
        CharacterClassType::Paladin,
        CharacterClassType::Psychic,
        CharacterClassType::Ranger,
        CharacterClassType::Shaman,
        CharacterClassType::Spiritualist
    })
    {
        checkConstruction<PreparedSpellcasting>(list);
        checkConstruction<SpontaneousSpellcasting>(list);
        checkConstruction<FlexibleSpellcasting>(list);
    }

    for (const int invalid : {-1, 999, static_cast<int>(CharacterClassType::Fighter)})
    {
        rejectsInvalidList<PreparedSpellcasting>(invalid);
        rejectsInvalidList<SpontaneousSpellcasting>(invalid);
        rejectsInvalidList<FlexibleSpellcasting>(invalid);
    }
}
