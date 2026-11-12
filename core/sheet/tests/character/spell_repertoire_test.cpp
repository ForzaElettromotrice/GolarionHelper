#include "golarion/character/spellcastings.hpp"
#include "golarion/character/spell_catalog.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/persistence/json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>

namespace
{
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
    SpellCatalog catalog;
    auto custom = catalog.get("fireball");
    custom.id = "custom.second";
    catalog.add(custom);
    ResourceManager resources;
    Spellcastings castings(resources, &catalog);
    for (const auto &[id, mode] : {std::pair{"p", SpellcastingMode::Prepared}, {"s", SpellcastingMode::Spontaneous}})
    {
        resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{id, "Class", mode, CharacterClassType::Wizard});
        resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"daily", "Class", {{3, 3}}});
        resources.addToCollection(repertoireQuotasResource(id), SpellRepertoireQuota{"known", "Class", {{2, 1}, {3, 1}}});
    }
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("p"));
    auto &spontaneous = dynamic_cast<SpontaneousSpellcasting &>(castings.get("s"));
    rejects([&] { spontaneous.addToRepertoire("fireball"); });
    rejects([&] { spontaneous.selectRepertoireSpell({"known", 2, "fireball"}); });
    rejects([&] { spontaneous.selectRepertoireSpell({"missing", 3, "fireball"}); });
    spontaneous.selectRepertoireSpell({"known", 3, "fireball"});
    rejects([&] { spontaneous.selectRepertoireSpell({"known", 3, "custom.second"}); });
    spontaneous.consume({"daily", 3, 0}, {"fireball"});
    prepared.selectRepertoireSpell({"known", 3, "fireball"});
    prepared.addToRepertoire("fireball");
    prepared.removeFromRepertoire("fireball");
    prepared.prepare({"daily", 3, 2}, {"fireball"}); // The quota still supplies it.
    prepared.clear({"daily", 3, 2});
    prepared.addToRepertoire("custom.second");
    prepared.prepare({"daily", 3, 0}, {"custom.second"});
    assert(castings.toView().castings[0].repertoireDetails.manualAdditionAllowed);
    assert(!castings.toView().castings[1].repertoireDetails.manualAdditionAllowed);
    for (const auto &choice : castings.pendingChoices())
        if (choice.type() == PendingChoiceType::Repertoire)
        {
            const auto &reference = std::get<RepertoireChoiceReference>(choice.choiceReference);
            assert(reference.quotaId == "known" && reference.spellLevel == 2);
            assert(choice.remaining == 1 && choice.blocksAdvancement);
        }

    // Provider revocation preserves choices and another provider can keep a spell known.
    resources.addToCollection(repertoireGrantsResource("s"), SpellRepertoireGrant{"bloodline", "Bloodline", {"fireball"}});
    resources.removeFromCollection(repertoireQuotasResource("s"), "known");
    assert(!castings.toView().castings[1].repertoireDetails.selections[0].active);
    spontaneous.consume({"daily", 3, 1}, {"fireball"});
    resources.removeFromCollection(repertoireGrantsResource("s"), "bloodline");
    rejects([&] { spontaneous.consume({"daily", 3, 2}, {"fireball"}); });
    auto saved = castings.toData();
    castings.load(saved);
    assert(castings.toView().castings[1].repertoireDetails.selections.size() == 1);
    resources.addToCollection(repertoireQuotasResource("s"), SpellRepertoireQuota{"known", "Class", {{3, 2}}});
    spontaneous.selectRepertoireSpell({"known", 3, "custom.second"});
    resources.removeFromCollection(repertoireQuotasResource("s"), "known");
    resources.addToCollection(repertoireQuotasResource("s"), SpellRepertoireQuota{"known", "Class", {{3, 1}}});
    for (const auto &selection : castings.toView().castings[1].repertoireDetails.selections) assert(!selection.active);
    spontaneous.removeRepertoireSelection({"known", 3, "custom.second"});
    spontaneous.consume({"daily", 3, 2}, {"fireball"});

    // Filtering and validation are independent from daily slots.
    resources.addToCollection(repertoireQuotasResource("s"), SpellRepertoireQuota{"filtered", "Feat", {{3, 1}}, std::set<std::string>{"custom.second"}});
    rejects([&] { spontaneous.selectRepertoireSpell({"filtered", 3, "fireball"}); });
    spontaneous.selectRepertoireSpell({"filtered", 3, "custom.second"});
    rejects([&] { resources.addToCollection(repertoireQuotasResource("s"), SpellRepertoireQuota{"bad", "Test", {{10, 1}}}); });
    rejects([&] { resources.addToCollection(repertoireQuotasResource("s"), SpellRepertoireQuota{"bad", "Test", {{3, 0}}}); });
    rejects([&] { resources.addToCollection(repertoireGrantsResource("s"), SpellRepertoireGrant{"bad", "Test", {"missing"}}); });

    // Whole-list knowledge remains dynamic, including later custom definitions.
    resources.addToCollection(repertoireListsResource("p"), SpellRepertoireListGrant{"list", "Class"});
    custom.id = "custom.later";
    catalog.add(custom);
    prepared.prepare({"daily", 3, 1}, {"custom.later"});
    prepared.clear({"daily", 3, 1});
    resources.removeFromCollection(repertoireListsResource("p"), "list");
    rejects([&] { prepared.prepare({"daily", 3, 1}, {"custom.later"}); });
    prepared.removeFromRepertoire("custom.second");
    castings.load(castings.toData()); // Previously prepared spell survives losing knowledge.
    prepared.clear({"daily", 3, 0});

    // Modern state round-trips source IDs; legacy spontaneous knowledge is retained for reassignment.
    auto sheet = CharacterSheet().toSaveData();
    sheet.spellcastings = castings.toData();
    const auto wire = persistence::toJson(sheet);
    const auto decoded = persistence::fromJson(wire);
    assert(persistence::toJson(decoded) == wire);
    castings.load(decoded.spellcastings);
    auto legacy = castings.toData();
    legacy.castings[1].repertoireData.reset();
    legacy.castings[1].repertoire = {"fireball"};
    castings.load(legacy);
    assert(castings.toView().castings[1].repertoire.empty());
    assert(castings.toView().castings[1].repertoireDetails.unassignedSpellIds == std::vector<std::string>{"fireball"});
    spontaneous.selectRepertoireSpell({"known", 3, "fireball"});
    assert(castings.toView().castings[1].repertoireDetails.unassignedSpellIds.empty());

    // Every new effect round-trips and removes only its own contribution.
    const std::vector<EffectDefinition> effects{
        SpellRepertoireQuotaEffectDefinition{"quota", "s", {{3, 2}}, std::set<std::string>{"fireball"}},
        SpellRepertoireGrantEffectDefinition{"grant", "s", {"custom.second"}},
        SpellRepertoireListEffectDefinition{"list", "p"}};
    for (const auto &effect : effects)
    {
        const auto json = effectDefinitionToJson(effect);
        const auto parsed = effectDefinitionFromJson(json);
        assert(effectDefinitionToJson(parsed) == json);
        const auto cleanup = compileEffect(parsed, {effectId(effect), "Test"})(resources);
        cleanup();
    }
    auto oldWire = nlohmann::json::parse(wire);
    oldWire["formatVersion"] = 39;
    for (auto &entry : oldWire["spellcastings"]) entry["repertoire"] = {"fireball"};
    const auto oldData = persistence::fromJson(oldWire.dump());
    assert(!oldData.spellcastings.castings[0].repertoireData);
    castings.load(oldData.spellcastings);
    assert(castings.toView().castings[0].repertoireDetails.manualSpellIds == std::vector<std::string>{"fireball"});
    assert(castings.toView().castings[1].repertoireDetails.unassignedSpellIds == std::vector<std::string>{"fireball"});
    auto invalid = castings.toData();
    invalid.castings[1].repertoireData->manualSpellIds = {"fireball"};
    rejects([&] { castings.load(invalid); });
    assert(castings.toView().castings[1].repertoire.empty());

    auto malformed = effectDefinitionToJson(effects[0]);
    malformed["quotas"][0]["capacity"] = 0.5;
    rejects([&] { effectDefinitionFromJson(malformed); });
}
