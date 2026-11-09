#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/character/character_sheet.hpp"
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
    const auto catalog = Json::parse(R"([{
        "id":"testItem", "name":"Oggetto di prova", "weightGrams":0, "volumeMilliliters":0,
        "tags":[], "slot":"ring", "usagePools":[
            {"id":"charges", "capacity":50},
            {"id":"daily", "capacity":3},
            {"id":"weekly", "capacity":1}]
    }])");
    auto &manager = ItemDefinitionManager::instance();
    for (int error = 0; error < 8; ++error)
    {
        auto bad = catalog;
        auto &pool = bad[0]["usagePools"][0];
        switch (error)
        {
        case 0: pool["capacity"] = 0; break;
        case 1: pool["capacity"] = -1; break;
        case 2: pool["capacity"] = 1.5; break;
        case 3: pool["capacity"] = std::numeric_limits<std::uint64_t>::max(); break;
        case 4: pool["recovery"] = "unknown"; break;
        case 5: pool["id"] = " "; break;
        case 6: bad[0]["usagePools"].push_back(pool); break;
        case 7: bad[0]["usagePools"] = Json::object(); break;
        }
        const auto text = bad.dump();
        embedded::ItemDefinitionCatalogJson = text;
        rejects([&] { manager.get("testItem"); });
    }
    const auto text = catalog.dump();
    embedded::ItemDefinitionCatalogJson = text;
    assert(manager.get("testItem").usagePools.size() == 3);
    ItemInstance first({"first", "testItem"});
    ItemInstance second({"second", "testItem"});
    auto charges = first.usagePool(" charges ");
    assert(charges == first.usagePool("charges"));
    charges->consume(33);
    assert(charges->remaining() == 17);
    assert(second.usagePool("charges")->remaining() == 50);
    for (int cost : {0, -1, 18}) rejects([&] { charges->consume(cost); });
    assert(charges->remaining() == 17);
    charges->restore(2);
    assert(charges->remaining() == 19);
    charges->restore(std::numeric_limits<int>::max());
    assert(charges->remaining() == 50);
    charges->consume();
    charges->refill();
    assert(charges->remaining() == 50);
    rejects([&] { charges->restore(0); });
    rejects([&] { charges->restore(-1); });
    first.usagePool("daily")->consume(3);
    first.usagePool("daily")->refill();
    assert(first.usagePool("daily")->remaining() == 3);
    first.usagePool("weekly")->consume();
    first.usagePool("weekly")->refill();
    assert(first.usagePool("weekly")->remaining() == 1);
    rejects([&] { first.usagePool("missing"); });
    rejects([&] { ItemInstance stack({"stack", "testItem", 2}); });
    const std::map<std::string, int> remaining{{"charges", 17}, {"daily", 0}, {"weekly", 0}};
    ItemInstance restored({"restored", "testItem", 1, {}, remaining});
    assert(restored.usagePool("charges")->remaining() == 17);
    assert(restored.usagePool("daily")->remaining() == 0);
    for (int error = 0; error < 5; ++error)
    {
        auto bad = remaining;
        if (error == 0) bad["charges"] = -1;
        if (error == 1) bad["charges"] = 51;
        if (error == 2) bad.erase("charges");
        if (error == 3) bad["unknown"] = 1;
        if (error == 4) bad[" charges "] = 17;
        rejects([&] { ItemInstance invalid({"bad", "testItem", 1, {}, bad}); });
    }
    rejects([&] { UsagePool invalid(0); });

    ResourceManager resources;
    resources.registerTarget("str", [] { return 10; });
    CarryingCapacity carryingCapacity(resources);
    Encumbrance encumbrance(resources, carryingCapacity);
    Inventory inventory(resources);
    inventory.addItem({"item", "testItem", 1, {}, remaining});
    inventory.equip("item");
    inventory.unequip("item");
    inventory.addContainer({.id = "home", .name = "Casa"});
    inventory.moveItem("item", "home");
    inventory.moveItem("item");
    assert(inventory.toSaveData().items[0].remainingUses == remaining);
    auto sheet = CharacterSheet().toSaveData();
    sheet.inventory = inventory.toSaveData();
    const auto encoded = persistence::toJson(sheet);
    assert(persistence::fromJson(encoded).inventory.items[0].remainingUses == remaining);
    auto json = Json::parse(encoded);
    for (int error = 0; error < 5; ++error)
    {
        auto bad = json;
        if (error == 0) bad["inventory"]["items"][0].erase("remainingUses");
        if (error == 1) bad["inventory"]["items"][0]["remainingUses"] = nullptr;
        if (error == 2) bad["inventory"]["items"][0]["remainingUses"]["charges"] = -1;
        if (error == 3) bad["inventory"]["items"][0]["remainingUses"]["charges"] = 1.5;
        if (error == 4) bad["inventory"]["items"][0]["remainingUses"]["charges"] = std::numeric_limits<std::uint64_t>::max();
        rejects([&] { persistence::fromJson(bad.dump()); });
    }
    const auto path = std::filesystem::temp_directory_path() / ("golarion-item-usage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    persistence::saveToFile(sheet, path);
    assert(CharacterSheet::load(path).toSaveData().inventory.items[0].remainingUses == remaining);
    std::filesystem::remove(path);
    json["formatVersion"] = 27;
    json["inventory"]["items"][0].erase("remainingUses");
    assert(!persistence::fromJson(json.dump()).inventory.items[0].remainingUses);
}
