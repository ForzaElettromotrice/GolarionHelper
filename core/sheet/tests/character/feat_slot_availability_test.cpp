#include "../fixtures/advance_sheet.hpp"
#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
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
    resources.registerEnhanceableResource("initiative");
    Feats feats(resources);
    feats.setLevel(1);
    auto provider = std::make_shared<int>(1);
    std::weak_ptr<int> lifetime = provider;
    resources.addToCollection(FeatSlotsResource, FeatSlot{
        .id = "source.slot", .source = "Fonte di prova", .exempts = [provider](const auto &, const auto &) { return *provider == 1; }
    });
    provider.reset();
    feats.selectFeat("source.slot", "improvedInitiative");
    resources.removeFromCollection(FeatSlotsResource, "source.slot");
    assert(lifetime.expired()); // No retained source callbacks after withdrawal.
    assert(!feats.toView().slots[1].available && !feats.toView().feats[0].active);
    assert(resources.modifierTotal("initiative") == 0);
    rejects([&] { resources.removeFromCollection(FeatSlotsResource, "source.slot"); });
    rejects([&] { feats.selectFeat("source.slot", "improvedInitiative"); });
    rejects([&] { feats.selectFeat("base.feat.level1", "improvedInitiative"); }); // Retained acquisitions still count for repeatability.
    const auto saved = feats.toData();
    feats.removeFeat("source.slot");
    assert(feats.toView().slots.size() == 1);
    feats.load(saved);
    assert(!feats.toView().slots[1].available && !feats.toView().feats[0].active);
    resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "source.slot", .source = "Fonte modificata", .filter = FeatIdFilter{{"other"}}});
    feats.recheckPrerequisites();
    assert(feats.toView().slots[1].available && !feats.toView().feats[0].active);
    // A changed filter must not make a retained selection impossible to load.
    feats.removeFeat("source.slot");
    feats.load(saved);
    assert(!feats.toView().feats[0].active);
    resources.removeFromCollection(FeatSlotsResource, "source.slot");
    resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "source.slot", .source = "Fonte ripristinata"});
    feats.recheckPrerequisites();
    assert(feats.toView().feats[0].active && resources.modifierTotal("initiative") == 4);
    resources.removeFromCollection(FeatSlotsResource, "source.slot");
    feats.removeFeat("source.slot");
    resources.addToCollection(FeatSlotsResource, FeatSlot{.id = "source.slot", .source = "Fonte ripristinata"});
    assert(!feats.toView().slots[1].featId); // Explicit deletion is not undone by a returning source.
    resources.removeFromCollection(FeatSlotsResource, "source.slot");
    rejects([&] { feats.load({{{"missing.slot", "unknownFeat", {}}}}); });
    assert(feats.toView().slots.size() == 1 && feats.toData().selections.empty());
    rejects([&] { feats.load({{saved.selections[0], saved.selections[0]}}); });
    assert(feats.toView().slots.size() == 1 && feats.toData().selections.empty());
    feats.load({{{"base.feat.level5", "improvedInitiative", {}}}});
    assert(!feats.toView().feats[0].active);
    feats.setLevel(5);
    assert(feats.toView().feats[0].active && resources.modifierTotal("initiative") == 4);
    feats.setLevel(1);
    assert(!feats.toView().feats[0].active && feats.toData().selections.size() == 1);
    feats.removeFeat("base.feat.level5");
    assert(feats.toView().slots.size() == 1);

    auto definition = test::classDefinition(CharacterClassType::Fighter);
    definition.features = {{.id = "training", .name = "Addestramento", .description = "Fonte sintetica.", .progression = {
        {.id = "first", .level = 1, .effects = {FeatSlotGrantEffectDefinition{"bonus"}}},
        {.id = "second", .level = 2, .replaces = {"first"}}
    }}};
    const ClassDefinitionManager catalog({definition});
    CharacterSheet sheet(catalog);
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 4);
    std::string slotId;
    const auto initial = sheet.toView();
    for (const auto &slot : initial.feats.slots) if (!slot.id.starts_with("base.")) slotId = slot.id;
    assert(!slotId.empty());
    sheet.selectFeat(slotId, "improvedInitiative");
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 4);
    assert(!sheet.toView().feats.feats[0].active);
    const auto path = std::filesystem::temp_directory_path() / "golarion_unavailable_feat_slot_test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(restored.toView().feats.feats.size() == 1 && !restored.toView().feats.feats[0].active);
    assert(restored.toView().feats.feats[0].slotId == slotId);
    assert(persistence::toJson(sheet.toSaveData()) == persistence::toJson(restored.toSaveData()));
    restored.removeFeat(slotId);
    assert(restored.toView().feats.feats.empty());
    std::filesystem::remove(path);
}
