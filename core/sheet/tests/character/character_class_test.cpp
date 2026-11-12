#include "golarion/character/character_class.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/character/spellcastings.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }

    golarion::ClassDefinition definition()
    {
        using namespace golarion;
        return {
            .type = CharacterClassType::Fighter,
            .name = "Classe di prova",
            .description = "Dati sintetici per i test.",
            .hitDie = ClassHitDie::D10,
            .skillRanksPerLevel = 2,
            .classSkills = {SkillType::Climb},
            .progression = {
                {1, 1, {{SavingThrowType::Fortitude, 2}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}},
                {2, 2, {{SavingThrowType::Fortitude, 3}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}},
                {3, 3, {{SavingThrowType::Fortitude, 3}, {SavingThrowType::Reflex, 1}, {SavingThrowType::Will, 1}}},
                {4, 4, {{SavingThrowType::Fortitude, 4}, {SavingThrowType::Reflex, 1}, {SavingThrowType::Will, 1}}}
            },
            .features = {{
                .id = "path", .name = "Percorso", .description = "Scelte e progressione.",
                .progression = {
                    {
                        .id = "first", .level = 1,
                        .effects = {
                            ContributionEffectDefinition{"scale", "bonus", "@classLevel * 2"},
                            FeatSlotGrantEffectDefinition{"slot"}
                        },
                        .choices = {{
                            .id = "path", .prompt = "Scegli un percorso", .selectionCount = 1,
                            .options = {
                                {.id = "a", .name = "Primo", .description = "Prima opzione", .progression = {
                                    {.id = "a.initial", .level = 1, .effects = {ContributionEffectDefinition{"a", "choice", "5"}}},
                                    {.id = "a.later", .level = 2, .effects = {ContributionEffectDefinition{"a", "choice", "7"}}}
                                }},
                                {.id = "b", .name = "Secondo", .description = "Seconda opzione", .progression = {
                                    {.id = "b.initial", .level = 1, .effects = {ContributionEffectDefinition{"b", "choice", "9"}}}
                                }}
                            }
                        }}
                    },
                    {.id = "upgrade", .level = 3, .effects = {ContributionEffectDefinition{"value", "bonus", "30"}}, .replaces = {"first"}},
                    {.id = "upgrade.again", .level = 4, .effects = {ContributionEffectDefinition{"value", "bonus", "40"}}, .replaces = {"upgrade"}}
                }
            }}
        };
    }
}

int main()
{
    using namespace golarion;
    static_assert(!std::is_copy_constructible_v<CharacterClass>);
    static_assert(!std::is_move_constructible_v<CharacterClass>);
    ResourceManager resources;
    for (const auto *name : {"baseAttackBonus", "bonus", "choice", "hp.max", "skill.ranks.max"}) resources.registerAccumulatedResource(name);
    resources.registerTarget("intMod", [] { return 0; });
    resources.registerTarget("intPermanentMod", [] { return 0; });
    resources.registerTarget("intSkillRanksMod", [] { return 0; });
    resources.registerTarget("conMod", [] { return 0; });
    for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will}) resources.registerAccumulatedResource(baseResourceName(type));
    int skillCount = 0, slotAdds = 0, slotRemoves = 0;
    resources.registerCollectionResource<SkillClassSkillGrant>(SkillClassSkillGrantsResource,
        [&](SkillClassSkillGrant) { ++skillCount; }, [&](std::string_view) { --skillCount; });
    resources.registerCollectionResource<FeatSlot>(FeatSlotsResource,
        [&](FeatSlot) { ++slotAdds; }, [&](std::string_view) { ++slotRemoves; });
    {
        CharacterClass first(resources, "first", definition());
        assert(resources.targetValue("class.first.level") == 1);
        assert(resources.contributionTotal("baseAttackBonus") == 1);
        assert(resources.contributionTotal(baseResourceName(SavingThrowType::Fortitude)) == 2);
        assert(resources.contributionTotal("bonus") == 2);
        assert(skillCount == 1 && slotAdds == 1);
        rejects([&] { CharacterClass duplicate(resources, "first", definition()); });
        assert(resources.targetValue("class.first.level") == 1);
        first.selectOption({"first", "path", "a"});
        assert(resources.contributionTotal("choice") == 5);
        rejects([&] { first.selectOption({"first", "path", "a"}); });
        rejects([&] { first.selectOption({"first", "path", "b"}); });
        rejects([&] { first.selectOption({"unknown", "path", "a"}); });
        rejects([&] { first.unselectOption({"first", "path", "b"}); });
        first.setLevel(2);
        first.setLevel(2);
        assert(resources.contributionTotal("choice") == 12);
        assert(resources.contributionTotal("bonus") == 4);
        assert(resources.contributionTotal("baseAttackBonus") == 2);
        assert(slotAdds == 1 && slotRemoves == 0);
        rejects([&] { first.setLevel(5); });
        rejects([&] { first.setLevel(-1); });
        assert(resources.targetValue("class.first.level") == 2);
        first.setLevel(3);
        assert(resources.contributionTotal("choice") == 0);
        assert(resources.contributionTotal("bonus") == 30);
        rejects([&] { first.selectOption({"first", "path", "b"}); });
        first.setLevel(4, 7);
        assert(first.toData().hitDieResults.at(4) == 7);
        assert(resources.contributionTotal("hp.max") == 7);
        assert(resources.contributionTotal("bonus") == 40);
        assert(resources.contributionTotal("choice") == 0);
        first.setLevel(2);
        assert(first.toData().hitDieResults.empty());
        assert(resources.contributionTotal("hp.max") == 0);
        assert(resources.contributionTotal("choice") == 12);
        first.unselectOption({"first", "path", "a"});
        assert(resources.contributionTotal("choice") == 0);
        first.selectOption({"first", "path", "b"});
        assert(resources.contributionTotal("choice") == 9);
        {
            CharacterClass second(resources, "second", definition(), 1);
            assert(resources.contributionTotal("baseAttackBonus") == 3);
            assert(resources.contributionTotal("bonus") == 6);
            assert(skillCount == 2);
        }
        first.setLevel(0);
        assert(resources.contributionTotal("baseAttackBonus") == 0 && skillCount == 0);
        first.setLevel(1);
        assert(resources.contributionTotal("choice") == 9);
    }
    assert(skillCount == 0 && slotAdds == slotRemoves);
    assert(resources.contributionTotal("bonus") == 0);
    rejects([&] { resources.targetValue("class.first.level"); });
    // The same instance ID can be reused after destruction.
    { CharacterClass reused(resources, "first", definition()); }

    auto failing = definition();
    failing.features[0].progression.push_back({.id = "failure", .level = 2, .effects = {
        ContributionEffectDefinition{"good", "bonus", "100"},
        ContributionEffectDefinition{"bad", "missing", "1"}
    }});
    {
        CharacterClass instance(resources, "failure", failing);
        instance.setHitDieResult(1, 6);
        instance.selectOption({"first", "path", "a"});
        const int adds = slotAdds, removes = slotRemoves;
        rejects([&] { instance.setLevel(2, 8); });
        assert(instance.toData().hitDieResults.size() == 1 && instance.toData().hitDieResults.at(1) == 6);
        assert(resources.contributionTotal("hp.max") == 6);
        assert(resources.targetValue("class.failure.level") == 1);
        assert(resources.contributionTotal("bonus") == 2);
        assert(resources.contributionTotal("choice") == 5);
        assert(slotAdds == adds && slotRemoves == removes);
    }
    rejects([&] { CharacterClass instance(resources, "failedConstructor", failing, 2); });
    rejects([&] { resources.targetValue("class.failedConstructor.level"); });
    assert(resources.contributionTotal("bonus") == 0 && skillCount == 0);
    assert(resources.contributionTotal("hp.max") == 0 && resources.contributionTotal("skill.ranks.max") == 0);
    auto invalid = definition();
    invalid.features[0].progression[1].replaces = {"missing"};
    rejects([&] { CharacterClass instance(resources, "invalid", invalid); });
    invalid = definition();
    invalid.features[0].progression[0].replaces = {"upgrade"};
    rejects([&] { CharacterClass instance(resources, "invalid", invalid); });
    invalid = definition();
    invalid.features[0].progression[1].id = "first";
    rejects([&] { CharacterClass instance(resources, "invalid", invalid); });
    {
        Spellcastings castings(resources);
        auto casterDefinition = definition();
        casterDefinition.features = {{"casting", "Incantesimi", "Capacità di lancio", {
            {.id = "initial", .level = 1, .effects = {SpellcastingGrantEffectDefinition{"casting", SpellcastingMode::Prepared, CharacterClassType::Wizard}}}
        }}};
        {
            CharacterClass caster(resources, "caster", casterDefinition);
            const auto id = castings.toView().castings.front().id;
            auto *casting = &castings.get(id);
            casting->addToRepertoire("fireball");
            caster.setLevel(2);
            caster.setLevel(1);
            assert(&castings.get(id) == casting);
            assert(castings.toData().castings.front().repertoire == std::vector<std::string>{"fireball"});
            caster.setLevel(0);
            assert(castings.toView().castings.empty());
        }
        assert(castings.toView().castings.empty());
    }
    return 0;
}
