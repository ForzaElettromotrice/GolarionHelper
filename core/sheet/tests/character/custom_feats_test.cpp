#include "../fixtures/advance_sheet.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "../fixtures/class_definition.hpp"
#include "../fixtures/definition_roundtrip.hpp"
#include <filesystem>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
    FeatDefinition customFeat()
    {
        return featDefinitionFromJson(Json::parse(R"({
            "id":"custom.gift","name":"Dono personale","description":"Talento personalizzato","types":["general","combat"],
            "requirements":[{"description":"Forza elevata","expression":"@str >= 30"},
                {"description":"Talento richiesto","feat":{"id":"missing"}}],
            "effects":[{"id":"hp","type":"contribution","resource":"hp.max","expression":"3"},
                {"id":"slot","type":"featSlotGrant"}],
            "choices":[{"id":"gift","prompt":"Scegli","selectionCount":1,"options":[
                {"id":"health","name":"Vigore","requirements":[{"description":"Requisito non risolvibile","expression":"@missing >= 1"}],
                    "effects":[{"id":"hp","type":"contribution","resource":"hp.max","expression":"2"}]},
                {"id":"other","name":"Altro","effects":[]}
            ]}]
        })"));
    }
}

int main()
{
    using namespace golarion;
    const FeatChoices choices{{"gift", {"health"}}};
    auto definition = customFeat();
    const auto encoded = featDefinitionToJson(definition);
    assert(featDefinitionToJson(featDefinitionFromJson(encoded)) == encoded);
    auto dependent = featDefinitionFromJson(Json::parse(R"({
        "id":"custom.dependent","name":"Dono successivo","description":"Dipendenza","types":["general"],
        "requirements":[{"description":"Stessa scelta","feat":{"id":"custom.gift","matchingChoices":{"gift":"gift"}}}],
        "choices":[{"id":"gift","prompt":"Scelta","selectionCount":1,"options":[{"id":"health","name":"Vigore"}]}],
        "effects":[{"id":"hp","type":"contribution","resource":"hp.max","expression":"7"}]
    })"));
    assert(featDefinitionToJson(featDefinitionFromJson(featDefinitionToJson(dependent))) == featDefinitionToJson(dependent));
    const auto standard = FeatDefinitionManager::instance().get("improvedInitiative");
    assert(featDefinitionToJson(featDefinitionFromJson(featDefinitionToJson(standard))) == featDefinitionToJson(standard));
    FeatCatalog local;
    local.add(definition);
    assert(&local.get("improvedInitiative") == &FeatDefinitionManager::instance().get("improvedInitiative"));
    rejects([&] { FeatCatalog{}.get(definition.id); });
    rejects([&] { FeatDefinitionManager::instance().get(definition.id); });
    rejects([&] { local.add(definition); });
    auto invalid = definition; invalid.id = " improvedInitiative ";
    rejects([&] { local.add(invalid); });
    invalid.id = "invalid"; invalid.types = {static_cast<FeatType>(999)};
    rejects([&] { local.add(invalid); });
    invalid = definition; invalid.id = "invalid"; invalid.choices[0].selectionCount = 0;
    rejects([&] { local.add(invalid); });
    invalid = definition; invalid.id = "invalid"; invalid.repeatability = static_cast<FeatRepeatability>(999);
    rejects([&] { local.add(invalid); });
    invalid = definition; invalid.id = "invalid"; invalid.effects.push_back(invalid.effects.front());
    rejects([&] { local.add(invalid); });
    assert(local.customDefinitions().size() == 1);

    ClassDefinitionManager classes({test::levelOnlyClassDefinition()});
    CharacterSheet sheet(classes);
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 8);
    const auto baseHp = sheet.toView().hitPoints.max;
    sheet.addCustomFeatDefinition(definition);
    sheet.addCustomFeatDefinition(dependent);
    assert(sheet.toView().customFeatDefinitions.size() == 2);
    assert(sheet.toView().feats.feats.empty());
    rejects([&] { sheet.selectFeat("base.feat.level1", definition.id, choices); });
    sheet.addExtraFeatSlot("extra.gift");
    sheet.addExtraFeatSlot("extra.empty");
    rejects([&] { sheet.addExtraFeatSlot(" extra.gift "); });
    rejects([&] { sheet.addExtraFeatSlot("base.feat.level1"); });
    rejects([&] { sheet.addExtraFeatSlot(" "); });
    rejects([&] { sheet.removeExtraFeatSlot("base.feat.level1"); });
    rejects([&] { sheet.selectFeat("extra.gift", definition.id); });
    rejects([&] { sheet.selectFeat("extra.gift", definition.id, {{"gift", {"missing"}}}); });
    sheet.selectFeat("extra.gift", definition.id, choices);
    assert(sheet.toView().hitPoints.max == baseHp + 5);
    rejects([&] { sheet.selectFeat("extra.empty", definition.id, choices); }); // Repeatability still applies.
    sheet.selectFeat("base.feat.level1", dependent.id, choices);
    assert(sheet.toView().hitPoints.max == baseHp + 12);
    sheet.setAbilityBaseValue(AbilityType::Strength, 1);
    for (const auto &feat : sheet.toView().feats.feats) assert(feat.active);
    bool foundExtra = false;
    for (const auto &slot : sheet.toView().feats.slots)
        if (slot.id == "extra.gift") { assert(slot.origin == FeatSlotOrigin::Extra && slot.available); foundExtra = true; }
    assert(foundExtra);
    const auto path = std::filesystem::temp_directory_path() / "golarion-custom-feats-test.json";
    const auto before = persistence::toJson(sheet.toSaveData());
    sheet.save(path);
    auto restored = CharacterSheet::load(path, classes);
    assert(persistence::toJson(restored.toSaveData()) == before);
    assert(restored.toView().hitPoints.max == baseHp + 12);
    assert(restored.toSaveData().feats.extraSlotIds.size() == 2);
    restored.removeFeat("extra.gift");
    assert(restored.toView().hitPoints.max == baseHp);
    assert(!restored.toView().feats.feats.front().active);
    assert(restored.toSaveData().feats.extraSlotIds.size() == 2);
    restored.selectFeat("extra.gift", definition.id, choices);
    assert(restored.toView().hitPoints.max == baseHp + 12);
    restored.removeExtraFeatSlot("extra.gift");
    assert(restored.toView().hitPoints.max == baseHp);
    assert(restored.toSaveData().feats.extraSlotIds == std::vector<std::string>{"extra.empty"});
    restored.removeExtraFeatSlot("extra.empty");
    assert(restored.toView().customFeatDefinitions.size() == 2);

    CharacterSheet isolated;
    isolated.addExtraFeatSlot("extra.standard");
    rejects([&] { isolated.selectFeat("extra.standard", definition.id, choices); });
    isolated.selectFeat("extra.standard", "improvedInitiative");
    assert(isolated.toView().feats.feats.front().active);
    isolated.addCustomFeatDefinition(definition);
    isolated.save(path);
    assert(CharacterSheet::load(path).toView().customFeatDefinitions.size() == 1);

    // User slots cannot be withdrawn or replaced through a provider endpoint.
    ResourceManager resources;
    resources.registerAccumulatedResource("hp.max");
    resources.registerTarget("str", [] { return 1; });
    Feats feats(resources, &local);
    feats.setLevel(3);
    feats.addExtraSlot("extra");
    rejects([&] { resources.removeFromCollection(FeatSlotsResource, "extra"); });
    rejects([&] { resources.addToCollection(FeatSlotsResource, FeatSlot{.id="extra", .source="Fonte"}); });
    feats.selectFeat("extra", definition.id, choices);
    feats.setLevel(0);
    assert(resources.contributionTotal("hp.max") == 5 && feats.toView().feats.front().active);
    feats.removeExtraSlot("extra");
    assert(resources.contributionTotal("hp.max") == 0);
    FeatsData bad;
    bad.extraSlotIds = {"one", " one "};
    rejects([&] { feats.load(bad); });
    assert(feats.toData().extraSlotIds.empty());
    bad.extraSlotIds = {"one"};
    bad.selections = {{"one", "missing", {}}};
    rejects([&] { feats.load(bad); });
    assert(feats.toView().slots.empty());

    auto malformed = Json::parse(before);
    malformed["customFeatDefinitions"] = Json::object();
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["feats"]["extraSlotIds"] = Json::object();
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["customFeatDefinitions"].push_back(malformed["customFeatDefinitions"][0]);
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, classes); });
    malformed = Json::parse(before); malformed["customFeatDefinitions"] = Json::array();
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, classes); });
    auto legacy = Json::parse(persistence::toJson(CharacterSheet{}.toSaveData()));
    legacy["formatVersion"] = 38;
    legacy.erase("customFeatDefinitions");
    legacy["feats"].erase("extraSlotIds");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    const auto old = CharacterSheet::load(path).toSaveData();
    assert(old.customFeatDefinitions.empty() && old.feats.extraSlotIds.empty());
    std::filesystem::remove(path);
}
