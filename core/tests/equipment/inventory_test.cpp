#include "golarion/equipment/inventory.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
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

    int possessionApplications = 0;
    int possessionCleanups = 0;
    int equippedApplications = 0;
    ItemDefinitionManager::instance().registerEffect("hempRope15m", ItemEffectDefinition{
        .id = "test.possession",
        .description = "Effetto mentre posseduto",
        .activation = ItemEffectActivation::Possessed,
        .apply = [&resourceManager, &possessionApplications, &possessionCleanups](ResourceManager &effectResourceManager, const ItemEffectContext &context)
        {
            assert(&effectResourceManager == &resourceManager);
            assert(!context.instanceId.empty());
            assert(context.itemDefinitionId == "hempRope15m");
            assert(context.effectId == "test.possession");
            assert(context.source == "Corda di canapa (15 m)");
            if (context.itemInstanceId == "rope.effect.active")
            {
                assert(context.quantity == 2);
            }
            else
            {
                assert(context.itemInstanceId == "rope.effect.inactive");
                assert(context.quantity == 1);
            }
            ++possessionApplications;
            return [&possessionCleanups]
            {
                ++possessionCleanups;
            };
        }
    });
    ItemDefinitionManager::instance().registerEffect("hempRope15m", ItemEffectDefinition{
        .id = "test.equipped",
        .description = "Effetto mentre equipaggiato",
        .activation = ItemEffectActivation::Equipped,
        .apply = [&equippedApplications](ResourceManager &, const ItemEffectContext &)
        {
            ++equippedApplications;
            return [] {};
        }
    });

    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.effect.active",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    });
    assert(possessionApplications == 1);
    assert(possessionCleanups == 0);
    assert(equippedApplications == 0);
    inventory.removeItem("rope.effect.active");
    assert(possessionCleanups == 1);

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
        .id = "rope.effect.inactive",
        .itemDefinitionId = "hempRope15m",
        .quantity = 1
    }, "item.backpack.home");
    assert(possessionApplications == 1);
    assert(possessionCleanups == 1);
    assert(equippedApplications == 0);
    assert(encumbrance.toView().totalWeightGrams == 0);

    inventory.moveItem("backpack.home");
    assert(possessionApplications == 2);
    assert(possessionCleanups == 1);
    assert(equippedApplications == 0);
    assert(encumbrance.toView().totalWeightGrams == 6000);
    inventory.moveItem("backpack.home");
    assert(possessionApplications == 2);
    assert(possessionCleanups == 1);
    inventory.moveItem("rope.effect.inactive");
    assert(possessionApplications == 2);
    assert(possessionCleanups == 1);
    assert(encumbrance.toView().totalWeightGrams == 6000);
    inventory.moveItem("rope.effect.inactive", "item.backpack.home");
    assert(possessionApplications == 2);
    assert(possessionCleanups == 1);
    inventory.moveItem("backpack.home", "home");
    assert(possessionApplications == 2);
    assert(possessionCleanups == 2);
    assert(encumbrance.toView().totalWeightGrams == 0);

    inventory.moveItem("rope.effect.inactive");
    assert(possessionApplications == 3);
    assert(possessionCleanups == 2);
    assert(encumbrance.toView().totalWeightGrams == 5000);
    inventory.moveItem("rope.effect.inactive", "item.backpack.home");
    assert(possessionApplications == 3);
    assert(possessionCleanups == 3);
    assert(encumbrance.toView().totalWeightGrams == 0);
    inventory.removeItem("rope.effect.inactive");
    inventory.removeItem("backpack.home");
    assert(possessionCleanups == 3);

    int rollbackApplications = 0;
    int rollbackCleanups = 0;
    ItemDefinitionManager::instance().registerEffect("bagOfHoldingTypeI", ItemEffectDefinition{
        .id = "test.rollback.first",
        .description = "Primo effetto transazionale",
        .activation = ItemEffectActivation::Possessed,
        .apply = [&rollbackApplications, &rollbackCleanups](ResourceManager &, const ItemEffectContext &)
        {
            ++rollbackApplications;
            return [&rollbackCleanups]
            {
                ++rollbackCleanups;
            };
        }
    });
    ItemDefinitionManager::instance().registerEffect("bagOfHoldingTypeI", ItemEffectDefinition{
        .id = "test.rollback.invalid",
        .description = "Cleanup non valido",
        .activation = ItemEffectActivation::Possessed,
        .apply = [&rollbackApplications](ResourceManager &, const ItemEffectContext &)
        {
            ++rollbackApplications;
            return ItemEffectCleanup{};
        }
    });
    inventory.addItem(ItemInstanceDefinition{
        .id = "bag.rollback",
        .itemDefinitionId = "bagOfHoldingTypeI",
        .quantity = 1
    }, "home");
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("bag.rollback");
    }));
    assert(rollbackApplications == 2);
    assert(rollbackCleanups == 1);
    assert(encumbrance.toView().totalWeightGrams == 0);
    inventory.removeItem("bag.rollback");

    return 0;
}
