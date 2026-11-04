#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/equipment/coin.hpp"
#include "golarion/equipment/inventory.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string_view>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }

    const golarion::ContainerMoneyView &containerMoney(const golarion::MoneyView &view, std::string_view containerId)
    {
        const auto container = std::ranges::find(view.containers, containerId, &golarion::ContainerMoneyView::containerId);
        assert(container != view.containers.end());
        return *container;
    }
}

int main()
{
    using namespace golarion;

    assert(displayName(CoinDenomination::Copper) == "Moneta di rame");
    assert(displayName(CoinDenomination::Silver) == "Moneta d'argento");
    assert(displayName(CoinDenomination::Gold) == "Moneta d'oro");
    assert(displayName(CoinDenomination::Platinum) == "Moneta di platino");
    assert(coinItemDefinitionId(CoinDenomination::Gold) == "goldCoin");
    const ItemDefinition &goldDefinition = ItemDefinitionManager::instance().get("goldCoin");
    assert(goldDefinition.coinDenomination == CoinDenomination::Gold);
    assert(goldDefinition.weightGrams == 10);

    ResourceManager resourceManager;
    resourceManager.registerTarget("str", []
    {
        return 30;
    });
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);
    Inventory inventory(resourceManager);
    inventory.addItem(ItemInstanceDefinition{
        .id = "bag.money",
        .itemDefinitionId = "bagOfHoldingTypeI",
        .quantity = 1
    });
    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.money",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    });

    inventory.addMoney(CoinDenomination::Gold, 10, "item.bag.money");
    inventory.addMoney(CoinDenomination::Gold, 10, "item.backpack.money");
    InventoryView inventoryView = inventory.toView();
    assert(inventoryView.money.containers.size() == 2);
    assert(containerMoney(inventoryView.money, "item.bag.money").amounts.gold == 10);
    assert(containerMoney(inventoryView.money, "item.bag.money").amounts.weightGrams == 100);
    assert(containerMoney(inventoryView.money, "item.backpack.money").amounts.gold == 10);
    assert(inventoryView.money.total.gold == 20);
    assert(inventoryView.money.total.weightGrams == 200);
    assert(encumbrance.toView().totalWeightGrams == 8600);

    inventory.addMoney(CoinDenomination::Gold, 5, "item.bag.money");
    inventoryView = inventory.toView();
    assert(containerMoney(inventoryView.money, "item.bag.money").amounts.gold == 15);
    assert(inventoryView.items.size() == 4);

    inventory.addItem(ItemInstanceDefinition{
        .id = "silver.direct",
        .itemDefinitionId = "silverCoin",
        .quantity = 3
    }, "item.bag.money");
    inventoryView = inventory.toView();
    assert(containerMoney(inventoryView.money, "item.bag.money").amounts.silver == 3);
    assert(inventoryView.money.total.silver == 3);

    inventory.removeMoney(CoinDenomination::Gold, 12, "item.bag.money");
    assert(containerMoney(inventory.toView().money, "item.bag.money").amounts.gold == 3);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeMoney(CoinDenomination::Silver, 4, "item.bag.money");
    }));
    assert(containerMoney(inventory.toView().money, "item.bag.money").amounts.silver == 3);

    inventory.removeMoney(CoinDenomination::Gold, 3, "item.bag.money");
    inventory.moveItem("silver.direct", "item.backpack.money");
    inventoryView = inventory.toView();
    assert(inventoryView.money.containers.size() == 1);
    assert(containerMoney(inventoryView.money, "item.backpack.money").amounts.gold == 10);
    assert(containerMoney(inventoryView.money, "item.backpack.money").amounts.silver == 3);

    inventory.addContainer(ContainerDefinition{
        .id = "carriedPurse",
        .name = "Borsello trasportato",
        .maximumContentsWeightGrams = 100,
        .acceptedItems = ItemSelector(ItemSelectorDefinition{.tags = {"coin"}}),
        .contributesToCarriedWeight = true
    });
    inventory.addMoney(CoinDenomination::Platinum, 10, "carriedPurse");
    assert(encumbrance.toView().totalWeightGrams == 8730);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addMoney(CoinDenomination::Platinum, 1, "carriedPurse");
    }));
    assert(containerMoney(inventory.toView().money, "carriedPurse").amounts.platinum == 10);
    const InventorySaveData inventoryData = inventory.toSaveData();
    const auto purseData = std::ranges::find(inventoryData.containers, "carriedPurse", &InventoryContainerSaveData::id);
    assert(purseData != inventoryData.containers.end());
    assert(purseData->contributesToCarriedWeight);

    CharacterSheet sheet;
    sheet.addMoney(CoinDenomination::Copper, 25, MainContainerId);
    assert(containerMoney(sheet.toView().inventory.money, MainContainerId).amounts.copper == 25);
    sheet.removeMoney(CoinDenomination::Copper, 5, MainContainerId);
    assert(containerMoney(sheet.toView().inventory.money, MainContainerId).amounts.copper == 20);
    const std::filesystem::path savePath = "money_test_save.json";
    sheet.save(savePath);
    CharacterSheet loaded = CharacterSheet::load(savePath);
    assert(containerMoney(loaded.toView().inventory.money, MainContainerId).amounts.copper == 20);
    std::filesystem::remove(savePath);

    return 0;
}
