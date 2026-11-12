#include "../fixtures/advance_sheet.hpp"
#include "golarion/character/character_sheet.hpp"
#include "../fixtures/class_definition.hpp"
#include "golarion/character/feat_definition_manager.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Function>
    void expectInvalid(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::logic_error &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    resources.registerEnhanceableResource("initiative");
    resources.registerTarget("bab", [] { return 3; });
    resources.registerTarget("int", [] { return 10; });
    const auto &initiative = FeatDefinitionManager::instance().get("improvedInitiative");
    {
        Feats feats(resources);
        feats.setLevel(5);
        assert(feats.toView().slots.size() == 3);
        feats.setLevel(5);
        assert(feats.toView().slots.size() == 3);
        feats.selectFeat("base.feat.level1", "improvedInitiative");
        assert(resources.modifierTotal("initiative") == 4);
        expectInvalid([&] { feats.selectFeat("base.feat.level3", "improvedInitiative"); });
        expectInvalid([&] { feats.selectFeat("base.feat.level1", "improvedInitiative"); });
        feats.removeFeat("base.feat.level1");
        assert(feats.toView().slots.size() == 3);
        assert(resources.modifierTotal("initiative") == 0);
        feats.selectFeat("base.feat.level5", "improvedInitiative");
        feats.setLevel(3);
        assert(feats.toView().slots.size() == 3);
        assert(!feats.toView().slots[2].available && !feats.toView().feats[0].active);
        assert(resources.modifierTotal("initiative") == 0);
        feats.setLevel(5);
        assert(feats.toView().slots.size() == 3);
        assert(feats.toView().slots[2].available && feats.toView().feats[0].active);
        assert(resources.modifierTotal("initiative") == 4);
        feats.removeFeat("base.feat.level5");
        expectInvalid([&] { feats.setLevel(-1); });

        resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "class.combat", .source = "Classe", .filter = FeatTypeFilter{FeatType::Combat}});
        resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "class.list", .source = "Classe", .filter = FeatIdFilter{{"improvedInitiative"}}});
        resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "class.other", .source = "Classe", .filter = FeatIdFilter{{"other"}}});
        assert(feats.qualifies("class.combat", initiative));
        assert(feats.qualifies("class.list", initiative));
        for (const auto &slot : feats.toView().slots)
        {
            if (slot.id == "class.combat") assert(slot.allowedType == FeatType::Combat);
            if (slot.id == "class.list") assert(slot.allowedFeatIds == std::vector<std::string>{"improvedInitiative"});
        }
        assert(!feats.qualifies("class.other", initiative));
        expectInvalid([&] { feats.selectFeat("class.other", "improvedInitiative"); });
        expectInvalid([&] { feats.selectFeat("missing", "improvedInitiative"); });
        expectInvalid([&] { feats.selectFeat("class.list", "missing"); });
        expectInvalid([&] { resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "class.list", .source = "Classe"}); });
        expectInvalid([&] { resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "base.feat.level9", .source = "Classe"}); });
        expectInvalid([&] { resources.removeFromCollection(FeatSlotsResource, "base.feat.level1"); });
        expectInvalid([&] { resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "empty", .source = "Classe", .filter = FeatIdFilter{}}); });
        feats.selectFeat("class.list", "improvedInitiative");
        const auto saved = feats.toData();
        assert(saved.selections.size() == 1);
        assert(saved.selections[0].slotId == "class.list");
        feats.removeFeat("class.list");
        feats.load(saved);
        assert(resources.modifierTotal("initiative") == 4);
        resources.removeFromCollection(FeatSlotsResource, "class.list");
        assert(resources.modifierTotal("initiative") == 0);
        assert(feats.toData().selections[0].featId == "improvedInitiative");
        feats.removeFeat("class.list");
        expectInvalid([&] { feats.removeFeat("class.list"); });

        auto restricted = initiative;
        restricted.requirements = {{"BAB richiesto", "@bab >= 6"}, {"Intelligenza richiesta", "@int >= 13"}};
        assert(!feats.qualifies("class.combat", restricted));
        resources.addToCollection(FeatPrerequisiteAlternativesResource, FeatPrerequisiteAlternative{
            .id = "babAlternative", .source = "Classe", .target = "bab",
            .applies = [](const auto &context) { return context.slotId == "class.combat"; },
            .calculate = [](const auto &, const auto &) { return 6; }
        });
        assert(!feats.qualifies("class.combat", restricted));
        resources.addToCollection(FeatPrerequisiteAlternativesResource, FeatPrerequisiteAlternative{
            .id = "intAlternative", .source = "Classe", .target = "int",
            .applies = [](const auto &) { return true; },
            .calculate = [](const auto &, const auto &) { return 13; }
        });
        assert(feats.qualifies("class.combat", restricted));
        assert(!feats.qualifies("base.feat.level1", restricted));
        restricted.requirements = {{"Riferimenti coerenti", "@bab >= 6 && @bab < 4"}};
        assert(!feats.qualifies("class.combat", restricted));
        restricted.requirements = {{"Valori combinati", "@bab >= 6 && @int >= 13"}};
        assert(feats.qualifies("class.combat", restricted));
        restricted.requirements = {{"Verifica manuale", std::nullopt}};
        assert(feats.qualifies("class.combat", restricted));

        resources.addToCollection(FeatSlotsResource, FeatSlot{
            .id = "exempt", .source = "Fonte speciale", .filter = FeatTypeFilter{FeatType::Combat},
            .exempts = [](const auto &, const auto &) { return true; }
        });
        assert(feats.qualifies("exempt", restricted));
        restricted.types = {FeatType::General};
        assert(!feats.qualifies("exempt", restricted));
        restricted.types = {FeatType::Combat};
        restricted.requirements.clear();
        restricted.choices = {{"weapon", "Scegli", 1, {{"dagger", "Pugnale", {{"Intelligenza richiesta", "@int >= 13"}}, {}}}}};
        assert(!feats.qualifies("class.combat", restricted));
        assert(feats.qualifies("class.combat", restricted, {{"weapon", {"dagger"}}}));
        assert(!feats.qualifies("class.combat", restricted, {{"weapon", {"missing"}}}));
        assert(!feats.qualifies("class.combat", restricted, {{"weapon", {"dagger", "dagger"}}}));
        resources.removeFromCollection(FeatPrerequisiteAlternativesResource, "intAlternative");
        assert(!feats.qualifies("class.combat", restricted, {{"weapon", {"dagger"}}}));
        // Leave an active effect for destructor cleanup.
        feats.selectFeat("class.combat", "improvedInitiative");
    }
    assert(resources.modifierTotal("initiative") == 0);
    // A failed effect application leaves the slot empty and reusable.
    ResourceManager incomplete;
    Feats retry(incomplete);
    retry.setLevel(1);
    expectInvalid([&] { retry.selectFeat("base.feat.level1", "improvedInitiative"); });
    assert(!retry.toView().slots[0].featId);
    incomplete.registerEnhanceableResource("initiative");
    retry.selectFeat("base.feat.level1", "improvedInitiative");
    assert(incomplete.modifierTotal("initiative") == 4);
    expectInvalid([&] { retry.load({}); });
    retry.removeFeat("base.feat.level1");
    const FeatSelectionData valid{"base.feat.level1", "improvedInitiative", {}};
    expectInvalid([&] { retry.load({{valid, valid}}); });
    assert(retry.toData().selections.empty());
    assert(incomplete.modifierTotal("initiative") == 0);
    retry.load({{{"missing", "improvedInitiative", {}}}});
    assert(!retry.toView().feats[0].active);
    retry.removeFeat("missing");
    expectInvalid([&] { retry.load({{{"base.feat.level1", "missing", {}}}}); });
    expectInvalid([&] { retry.load({{{"base.feat.level1", "improvedInitiative", {{"unknown", {"option"}}}}}}); });

    const ClassDefinitionManager classCatalog({test::classDefinition(CharacterClassType::Fighter)});
    CharacterSheet sheet(classCatalog);
    assert(sheet.level() == 0 && sheet.toView().feats.slots.empty());
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter);
    assert(sheet.toView().feats.slots.size() == 1);
    const int initial = sheet.toView().initiative.abilityOptions[0].totalValue;
    sheet.selectFeat("base.feat.level1", "improvedInitiative");
    assert(sheet.toView().initiative.abilityOptions[0].totalValue == initial + 4);
    const auto view = sheet.toView().feats;
    assert(view.feats.size() == 1);
    assert(view.feats[0].id == initiative.id);
    assert(view.feats[0].name == initiative.name);
    assert(view.feats[0].description == initiative.description);
    assert(view.slots[0].featId == initiative.id);
    assert(!view.slots[0].allowedType && view.slots[0].allowedFeatIds.empty());
    const auto json = persistence::toJson(sheet.toSaveData());
    const auto path = std::filesystem::temp_directory_path() / "golarion_feats_round_trip.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, classCatalog);
    assert(restored.toView().initiative.abilityOptions[0].totalValue == initial + 4);
    assert(restored.toView().feats.feats[0].id == initiative.id);
    restored.removeFeat("base.feat.level1");
    assert(restored.toView().initiative.abilityOptions[0].totalValue == initial);
    auto legacy = nlohmann::json::parse(json);
    legacy["formatVersion"] = 24;
    legacy.erase("feats");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    auto legacySheet = CharacterSheet::load(path);
    std::filesystem::remove(path);
    assert(legacySheet.toView().feats.feats.empty());
    // The wire format preserves choice IDs without serializing catalog text.
    auto data = sheet.toSaveData();
    data.feats.selections[0].choices = {{"weapon", {"dagger", "sword"}}};
    const auto roundTrip = persistence::fromJson(persistence::toJson(data));
    assert(roundTrip.feats.selections[0].choices == data.feats.selections[0].choices);
    sheet.removeFeat("base.feat.level1");
    assert(sheet.toView().initiative.abilityOptions[0].totalValue == initial);
    assert(sheet.toView().feats.slots.size() == 1);
    static_cast<void>(sheet.toSaveData());
}
