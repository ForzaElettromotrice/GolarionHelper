#include "golarion/equipment/inventory.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/encumbrance_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

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
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    resourceManager.registerTarget("str", []
    {
        return 30;
    });
    resourceManager.registerEnhanceableResource("savingThrow.all");
    resourceManager.registerEnhanceableResource("abilityCheck.all");
    resourceManager.registerEnhanceableResource("skill.all");
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);
    Inventory inventory(resourceManager);
    EncumbranceView encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 0);
    assert(encumbranceView.weights.size() == 1);
    assert(encumbranceView.weights[0].id == InventoryWeightId);
    assert(encumbranceView.weights[0].source == "Inventario");
    inventory.addContainer(ContainerDefinition{
        .id = "home",
        .name = "Casa",
        .allowsPossessionEffects = false
    });

    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.main",
        .itemDefinitionId = "hempRope15m",
        .quantity = 1
    });
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 5000);
    assert(encumbranceView.weights.size() == 1);
    inventory.moveItem("rope.main");
    assert(encumbrance.toView().totalWeightGrams == 5000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("missing");
    }));
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("rope.main", "missing");
    }));
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "rope.main",
            .itemDefinitionId = "hempRope15m",
            .quantity = 1
        });
    }));
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "rope.missing",
            .itemDefinitionId = "hempRope15m",
            .quantity = 1
        }, "missing");
    }));

    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.1",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    });
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 6000);
    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.backpack",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    }, "item.backpack.1");
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 16000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "rope.tooLarge",
            .itemDefinitionId = "hempRope15m",
            .quantity = 10
        }, "item.backpack.1");
    }));
    assert(encumbrance.toView().totalWeightGrams == 16000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "backpack.stack",
            .itemDefinitionId = "commonBackpack",
            .quantity = 2
        });
    }));

    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.nested",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    }, "item.backpack.1");
    assert(encumbrance.toView().totalWeightGrams == 17000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "rope.rejectedByAncestor",
            .itemDefinitionId = "hempRope15m",
            .quantity = 10
        }, "item.backpack.nested");
    }));
    assert(encumbrance.toView().totalWeightGrams == 17000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("backpack.1", "item.backpack.nested");
    }));
    assert(encumbrance.toView().totalWeightGrams == 17000);

    inventory.moveItem("rope.backpack");
    assert(encumbrance.toView().totalWeightGrams == 17000);
    inventory.moveItem("rope.backpack", "item.backpack.1");
    assert(encumbrance.toView().totalWeightGrams == 17000);

    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.capacity",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    }, "home");
    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.capacity",
        .itemDefinitionId = "hempRope15m",
        .quantity = 10
    }, "item.backpack.capacity");
    assert(encumbrance.toView().totalWeightGrams == 17000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("backpack.capacity", "item.backpack.1");
    }));
    assert(encumbrance.toView().totalWeightGrams == 17000);
    inventory.removeItem("rope.capacity");
    inventory.removeItem("backpack.capacity");

    inventory.addItem(ItemInstanceDefinition{
        .id = "bag.1",
        .itemDefinitionId = "bagOfHoldingTypeI",
        .quantity = 1
    });
    assert(encumbrance.toView().totalWeightGrams == 24500);
    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.hidden",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    }, "item.bag.1");
    assert(encumbrance.toView().totalWeightGrams == 24500);
    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.hidden",
        .itemDefinitionId = "hempRope15m",
        .quantity = 1
    }, "item.backpack.hidden");
    assert(encumbrance.toView().totalWeightGrams == 24500);
    assert(encumbrance.toView().weights.size() == 1);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("backpack.hidden", "item.backpack.hidden");
    }));
    inventory.moveItem("backpack.hidden");
    assert(encumbrance.toView().totalWeightGrams == 30500);
    inventory.moveItem("backpack.hidden", "item.bag.1");
    assert(encumbrance.toView().totalWeightGrams == 24500);

    const InventoryView inventoryView = inventory.toView();
    assert(inventoryView.totalCarriedWeightGrams == 24500);
    assert(inventoryView.items.size() == 7);
    assert(inventoryView.containers.size() == 6);
    assert(inventoryView.equipmentSlots.size() == 14);
    const auto bagView = std::ranges::find(inventoryView.items, "bag.1", &InventoryItemView::id);
    assert(bagView != inventoryView.items.end());
    assert(bagView->ownedContainerId == "item.bag.1");
    assert(bagView->effectiveWeightGrams == 7500);
    const auto hiddenBackpackView = std::ranges::find(inventoryView.items, "backpack.hidden", &InventoryItemView::id);
    assert(hiddenBackpackView != inventoryView.items.end());
    assert(hiddenBackpackView->containerId == "item.bag.1");
    assert(hiddenBackpackView->possessionEffectsActive);

    const InventorySaveData inventoryData = inventory.toSaveData();
    assert(inventoryData.containers.size() == 1);
    assert(inventoryData.containers[0].id == "home");
    assert(inventoryData.items.size() == 7);
    const auto hiddenBackpackData = std::ranges::find(inventoryData.items, "backpack.hidden", &InventoryItemSaveData::id);
    assert(hiddenBackpackData != inventoryData.items.end());
    assert(hiddenBackpackData->containerId == "item.bag.1");
    assert(!hiddenBackpackData->equipped);

    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeItem("backpack.1");
    }));
    assert(encumbrance.toView().totalWeightGrams == 24500);

    inventory.removeItem("rope.hidden");
    assert(encumbrance.toView().totalWeightGrams == 24500);
    inventory.removeItem("backpack.hidden");
    assert(encumbrance.toView().totalWeightGrams == 24500);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{
            .id = "rope.removedContainer",
            .itemDefinitionId = "hempRope15m",
            .quantity = 1
        }, "item.backpack.hidden");
    }));
    inventory.removeItem("bag.1");
    assert(encumbrance.toView().totalWeightGrams == 17000);

    inventory.removeItem("backpack.nested");
    assert(encumbrance.toView().totalWeightGrams == 16000);
    inventory.removeItem("rope.backpack");
    assert(encumbrance.toView().totalWeightGrams == 6000);
    inventory.removeItem("backpack.1");
    assert(encumbrance.toView().totalWeightGrams == 5000);
    inventory.removeItem("rope.main");
    assert(encumbrance.toView().totalWeightGrams == 0);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeItem("rope.main");
    }));

    inventory.addItem(ItemInstanceDefinition{
        .id = "stone.effect.active",
        .itemDefinitionId = "stoneOfGoodLuck",
        .quantity = 1
    });
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(resourceManager.modifierTotal("abilityCheck.all") == 1);
    assert(resourceManager.modifierTotal("skill.all") == 1);
    inventory.removeItem("stone.effect.active");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    assert(resourceManager.modifierTotal("abilityCheck.all") == 0);
    assert(resourceManager.modifierTotal("skill.all") == 0);

    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addContainer(ContainerDefinition{
            .id = "invalid.virtual",
            .name = "Virtuale non valido",
            .ownerItemId = "owner"
        });
    }));
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addContainer(ContainerDefinition{
            .id = "item.reserved",
            .name = "Namespace riservato"
        });
    }));
    inventory.addItem(ItemInstanceDefinition{
        .id = "backpack.home",
        .itemDefinitionId = "commonBackpack",
        .quantity = 1
    }, "home");
    inventory.addItem(ItemInstanceDefinition{
        .id = "stone.effect.inactive",
        .itemDefinitionId = "stoneOfGoodLuck",
        .quantity = 1
    }, "item.backpack.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    assert(encumbrance.toView().totalWeightGrams == 0);

    inventory.moveItem("backpack.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 1000);
    inventory.moveItem("backpack.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    inventory.moveItem("stone.effect.inactive");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 1000);
    inventory.moveItem("stone.effect.inactive", "item.backpack.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    inventory.moveItem("backpack.home", "home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    assert(encumbrance.toView().totalWeightGrams == 0);

    inventory.moveItem("stone.effect.inactive");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 0);
    inventory.moveItem("stone.effect.inactive", "item.backpack.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    assert(encumbrance.toView().totalWeightGrams == 0);
    inventory.removeItem("stone.effect.inactive");
    inventory.removeItem("backpack.home");

    ResourceManager rollbackResourceManager;
    rollbackResourceManager.registerTarget("str", []
    {
        return 10;
    });
    rollbackResourceManager.registerEnhanceableResource("savingThrow.all");
    CarryingCapacity rollbackCarryingCapacity(rollbackResourceManager);
    Encumbrance rollbackEncumbrance(rollbackResourceManager, rollbackCarryingCapacity);
    Inventory rollbackInventory(rollbackResourceManager);
    assert(throwsInvalidArgument([&rollbackInventory]
    {
        rollbackInventory.addItem(ItemInstanceDefinition{
            .id = "stone.rollback",
            .itemDefinitionId = "stoneOfGoodLuck",
            .quantity = 1
        });
    }));
    assert(rollbackResourceManager.modifierTotal("savingThrow.all") == 0);
    assert(rollbackInventory.toView().items.empty());
    assert(rollbackEncumbrance.toView().totalWeightGrams == 0);

    return 0;
}
