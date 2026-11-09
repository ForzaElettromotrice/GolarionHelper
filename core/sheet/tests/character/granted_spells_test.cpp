#include "golarion/character/character_sheet.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/persistence/json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <chrono>
#include <limits>

namespace
{
    template<typename F> void rejects(F function)
    {
        bool rejected = false;
        try { function(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    using Json = nlohmann::json;
    ResourceManager resources;
    const auto limitedJson = Json::parse(R"({"id":"magic","type":"spellGrant","spellId":"fireball","usage":{"type":"limited","capacity":2}})");
    {
        GrantedSpells spells(resources);
        rejects([&] { GrantedSpells duplicate(resources); });
        auto apply = compileEffect(effectDefinitionFromJson(limitedJson), {"race.magic", "Razza"});
        auto cleanup = apply(resources);
        assert(spells.toView()[0].remaining == 2);
        spells.consume(" race.magic ");
        const auto saved = spells.toData();
        assert(saved.spells[0].remaining == 1);
        spells.consume("race.magic");
        rejects([&] { spells.consume("race.magic"); });
        spells.restore("race.magic", 1);
        assert(spells.toView()[0].remaining == 1);
        spells.restore("race.magic", std::numeric_limits<int>::max());
        assert(spells.toView()[0].remaining == 2);
        spells.load(saved);
        assert(spells.toView()[0].remaining == 1);
        auto secondApply = compileEffect(effectDefinitionFromJson(limitedJson), {"race.second", "Razza"});
        auto secondCleanup = secondApply(resources);
        auto invalidPair = spells.toData();
        invalidPair.spells[0].remaining = 0;
        invalidPair.spells[1].remaining = 99;
        rejects([&] { spells.load(invalidPair); });
        assert(spells.toView()[0].remaining == 1 && spells.toView()[1].remaining == 2);
        secondCleanup();
        for (int error = 0; error < 5; ++error)
        {
            auto bad = saved;
            if (error == 0) bad.spells[0].remaining = -1;
            if (error == 1) bad.spells[0].remaining = 3;
            if (error == 2) bad.spells.push_back(bad.spells[0]);
            if (error == 3) bad.spells.clear();
            if (error == 4) bad.spells[0].id = "unknown";
            rejects([&] { spells.load(bad); });
            assert(spells.toView()[0].remaining == 1);
        }
        auto root = CharacterSheet().toSaveData();
        root.grantedSpells = saved;
        const auto encoded = persistence::toJson(root);
        assert(persistence::fromJson(encoded).grantedSpells.spells[0].remaining == 1);
        auto badWire = Json::parse(encoded);
        badWire["grantedSpells"][0]["remaining"] = 1.5;
        rejects([&] { persistence::fromJson(badWire.dump()); });
        badWire["formatVersion"] = 28;
        badWire.erase("grantedSpells");
        assert(persistence::fromJson(badWire.dump()).grantedSpells.spells.empty());
        rejects([&] { apply(resources); });
        cleanup();
        rejects([&] { spells.consume("race.magic"); });

        for (auto mode : {GrantedSpellUsage::AtWill, GrantedSpellUsage::Constant})
        {
            auto unlimited = limitedJson;
            unlimited["usage"] = {{"type", mode == GrantedSpellUsage::AtWill ? "atWill" : "constant"}};
            auto release = compileEffect(effectDefinitionFromJson(unlimited), {"free", "Test"})(resources);
            if (mode == GrantedSpellUsage::AtWill) { spells.consume("free"); spells.consume("free"); }
            else rejects([&] { spells.consume("free"); });
            assert(!spells.toView()[0].remaining);
            assert(spells.toData().spells.empty());
            rejects([&] { spells.refill("free"); });
            release();
        }
        rejects([&] { resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"bad", "Test", "missing", GrantedSpellUsage::AtWill}); });
        rejects([&] { resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"bad", "Test", "fireball", static_cast<GrantedSpellUsage>(999)}); });
        for (int error = 0; error < 8; ++error)
        {
            auto bad = limitedJson;
            if (error == 0) bad["usage"]["type"] = "unknown";
            if (error == 1) bad["usage"]["capacity"] = 0;
            if (error == 2) bad["usage"]["capacity"] = 1.5;
            if (error == 3) bad["usage"]["capacity"] = std::numeric_limits<std::uint64_t>::max();
            if (error == 4) bad["usage"]["poolId"] = "charges";
            if (error == 5) bad["usage"]["type"] = "atWill";
            if (error == 6) bad["usage"].erase("capacity");
            if (error == 7) bad["usage"]["cost"] = 1;
            rejects([&] { effectDefinitionFromJson(bad); });
        }
        const auto external = effectDefinitionFromJson(Json::parse(R"({"id":"item","type":"spellGrant","spellId":"fireball","usage":{"type":"sharedPool","poolId":"charges","cost":2}})"));
        rejects([&] { compileEffect(external, {"bad", "Test"}); });
        auto pool = std::make_shared<UsagePool>(3);
        auto sharedApply = compileEffect(external, {"shared", "Test", [pool](std::string_view) { return pool; }});
        auto revoke = sharedApply(resources);
        spells.consume("shared");
        assert(pool->remaining() == 1);
        rejects([&] { spells.consume("shared"); });
        rejects([&] { spells.load({{{"shared", 1}}}); });
        assert(spells.toData().spells.empty());
        revoke();
        revoke = sharedApply(resources);
        assert(spells.toView()[0].remaining == 1);
        revoke();
    }
    rejects([&] { resources.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{"free", "Test", "fireball", GrantedSpellUsage::AtWill}); });
    GrantedSpells recreated(resources);

    // Exercise the actual item compiler, activation lifecycle, sheet facade and save/load path.
    embedded::ItemDefinitionCatalogJson = R"([{
        "id":"testItem","name":"Oggetto di prova","description":"Ricaricare secondo le regole della descrizione.",
        "weightGrams":0,"volumeMilliliters":0,"tags":[],"slot":"ring",
        "usagePools":[{"id":"charges","capacity":3}],
        "effects":[
            {"id":"one","activation":"possessed","type":"spellGrant","spellId":"fireball","usage":{"type":"sharedPool","poolId":"charges"}},
            {"id":"two","activation":"equipped","type":"spellGrant","spellId":"fireball","usage":{"type":"sharedPool","poolId":"charges","cost":2}}
        ]
    }])";
    ResourceManager itemResources;
    GrantedSpells itemSpells(itemResources);
    itemResources.registerTarget("str", [] { return 10; });
    CarryingCapacity capacity(itemResources);
    Encumbrance encumbrance(itemResources, capacity);
    Inventory inventory(itemResources);
    inventory.addItem({"item", "testItem"});
    const auto firstId = itemSpells.toView()[0].id;
    inventory.equip("item");
    std::string secondId;
    for (const auto &spell : itemSpells.toView()) if (spell.cost == 2) secondId = spell.id;
    assert(!secondId.empty());
    itemSpells.consume(firstId);
    itemSpells.consume(secondId);
    assert(inventory.toSaveData().items[0].remainingUses->at("charges") == 0);
    rejects([&] { itemSpells.consume(firstId); });
    inventory.unequip("item");
    rejects([&] { itemSpells.consume(secondId); });
    inventory.equip("item");
    rejects([&] { itemSpells.consume(secondId); });
    inventory.restoreUsagePool("item", "charges", 1);
    assert(itemSpells.toView()[0].remaining == 1);
    auto sheetData = CharacterSheet().toSaveData();
    sheetData.inventory = inventory.toSaveData();
    assert(sheetData.grantedSpells.spells.empty());
    const auto path = std::filesystem::temp_directory_path() / ("golarion-granted-spells-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    persistence::saveToFile(sheetData, path);
    auto sheet = CharacterSheet::load(path);
    std::filesystem::remove(path);
    assert(sheet.toView().grantedSpells.size() == 2);
    assert(!sheet.toView().inventory.items[0].description.empty());
    sheet.consumeGrantedSpell(firstId);
    assert(sheet.toSaveData().inventory.items[0].remainingUses->at("charges") == 0);
    sheet.refillItemUsagePool("item", "charges");
    sheet.consumeGrantedSpell(secondId);
    sheet.restoreItemUsagePool("item", "charges", 1);
    assert(sheet.toSaveData().inventory.items[0].remainingUses->at("charges") == 2);
    inventory.unequip("item");
    inventory.removeItem("item");
    assert(itemSpells.toView().empty());
}
