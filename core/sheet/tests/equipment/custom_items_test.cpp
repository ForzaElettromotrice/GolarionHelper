#include "golarion/character/character_sheet.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/persistence/json.hpp"
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
    ItemDefinition customRing()
    {
        return itemDefinitionFromJson(Json::parse(R"({
            "id":"custom.ring","name":"Anello personalizzato","description":"Tre cariche, ricaricate dal giocatore.",
            "weightGrams":20,"volumeMilliliters":5,"tags":["magic"],"slot":"ring",
            "usagePools":[{"id":"charges","capacity":3}],
            "effects":[
                {"id":"power","activation":"equipped","type":"modifier","resource":"str","description":"Forza","modifierType":"bonus","bonusType":"enhancement","expression":"2"},
                {"id":"spell","activation":"equipped","type":"spellGrant","spellId":"fireball","usage":{"type":"sharedPool","poolId":"charges","cost":1}}
            ],
            "choices":[{"id":"gift","prompt":"Scegli un dono","selectionCount":1,"options":[
                {"id":"health","name":"Vigore","effects":[{"id":"hp","activation":"possessed","type":"contribution","resource":"hp.max","expression":"5"}]},
                {"id":"warning","name":"Avvertimento","effects":[{"id":"note","activation":"possessed","type":"reminder","message":"Dono personalizzato"}]}
            ]}]
        })"));
    }
}

int main()
{
    using namespace golarion;
    // All embedded definitions pass through the same bidirectional schema.
    for (const auto &source : Json::parse(embedded::ItemDefinitionCatalogJson))
    {
        auto definition = itemDefinitionFromJson(source);
        const auto encoded = itemDefinitionToJson(definition);
        assertJsonContains(encoded, source);
        assert(itemDefinitionToJson(itemDefinitionFromJson(encoded)) == encoded);
    }

    CharacterSheet sheet;
    const auto initialStrength = sheet.toView().abilities.front().totalValue;
    sheet.addCustomItemDefinition(customRing());
    assert(sheet.toView().customItemDefinitions.size() == 1);
    rejects([&] { sheet.addCustomItemDefinition(customRing()); });
    auto collision = customRing(); collision.id = " cloakOfResistance1 ";
    rejects([&] { sheet.addCustomItemDefinition(collision); });
    auto invalid = customRing(); invalid.id = "bad"; invalid.weightGrams = -1;
    rejects([&] { sheet.addCustomItemDefinition(invalid); });
    invalid = customRing(); invalid.id = "bad"; invalid.slot = static_cast<EquipmentSlot>(999);
    rejects([&] { sheet.addCustomItemDefinition(invalid); });
    assert(sheet.toSaveData().customItemDefinitions.size() == 1);
    CharacterSheet isolated;
    rejects([&] { isolated.addItem({"missing", "custom.ring", 1, {{"gift", {"health"}}}}); });
    isolated.addCustomItemDefinition(customRing()); // Same ID is allowed on another sheet.
    rejects([&] { sheet.addItem({"bad", "custom.ring"}); }); // Required choices are still enforced.
    sheet.addItem({"ring1", "custom.ring", 1, {{"gift", {"health"}}}});
    sheet.addItem({"ring2", "custom.ring", 1, {{"gift", {"warning"}}}});
    sheet.equipItem("ring1");
    sheet.equipItem("ring2");
    assert(sheet.toView().abilities.front().totalValue == initialStrength + 2);
    assert(sheet.toView().hitPoints.max == 5);
    auto grants = sheet.toView().grantedSpells;
    assert(grants.size() == 2);
    sheet.consumeGrantedSpell(grants.front().id);
    sheet.consumeGrantedSpell(grants.front().id);
    assert(sheet.toView().grantedSpells.front().remaining == 1);
    assert(sheet.toView().grantedSpells.back().remaining == 3);

    auto bag = itemDefinitionFromJson(Json::parse(R"({"id":"custom.bag","name":"Sacca","weightGrams":100,"volumeMilliliters":50,"tags":["bag"],
        "container":{"maximumContentsWeightGrams":2000,"maximumContentsVolumeMilliliters":500,"acceptedItems":{"tags":["magic"]},
        "quantityLimits":[{"maximumQuantity":2,"selector":{"definitionIds":["custom.ring"]}}],"ignoresContentsWeight":true,"allowsPossessionEffects":false}})"));
    sheet.addCustomItemDefinition(bag);
    sheet.addItem({"bag", "custom.bag"});
    sheet.unequipItem("ring2", "item.bag");
    const auto before = persistence::toJson(sheet.toSaveData());
    const auto path = std::filesystem::temp_directory_path() / "golarion-custom-items-test.json";
    sheet.save(path);
    {
        auto restored = CharacterSheet::load(path);
        assert(persistence::toJson(restored.toSaveData()) == before);
        assert(restored.toView().customItemDefinitions.size() == 2);
        assert(restored.toView().abilities.front().totalValue == initialStrength + 2);
        assert(restored.toView().grantedSpells.front().remaining == 1);
        restored.unequipItem("ring1");
        restored.equipItem("ring1");
        assert(restored.toView().grantedSpells.front().remaining == 1);
        restored.refillItemUsagePool("ring1", "charges");
        assert(restored.toView().grantedSpells.front().remaining == 3);
        restored.unequipItem("ring1"); restored.removeItem("ring1");
        assert(restored.toView().abilities.front().totalValue == initialStrength);
        assert(restored.toView().hitPoints.max == 0);
        restored.removeItem("ring2"); restored.removeItem("bag");
        assert(restored.toSaveData().customItemDefinitions.size() == 2);
        restored.save(path);
    }
    auto definitionsOnly = CharacterSheet::load(path);
    definitionsOnly.addItem({"new", "custom.ring", 1, {{"gift", {"health"}}}});
    assert(definitionsOnly.toView().inventory.items.front().usagePools.front().remaining == 3);

    auto malformed = Json::parse(before);
    malformed["customItemDefinitions"][0]["weightGrams"] = 1.5;
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["customItemDefinitions"] = Json::object();
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["customItemDefinitions"].push_back(malformed["customItemDefinitions"][0]);
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path); });
    malformed = Json::parse(before); malformed["customItemDefinitions"] = Json::array();
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path); });
    auto legacy = Json::parse(persistence::toJson(CharacterSheet{}.toSaveData()));
    legacy["formatVersion"] = 36; legacy.erase("customItemDefinitions");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    assert(CharacterSheet::load(path).toSaveData().customItemDefinitions.empty());
    std::filesystem::remove(path);
}
