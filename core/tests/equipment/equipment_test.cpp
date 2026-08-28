#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/equipment/inventory.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/encumbrance_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

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

    int equipmentApplications = 0;
    int equipmentCleanups = 0;
    ResourceManager resourceManager;
    resourceManager.registerTarget("str", []
    {
        return 10;
    });
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);
    Inventory inventory(resourceManager);
    inventory.addContainer(ContainerDefinition{
        .id = "home",
        .name = "Casa",
        .allowsPossessionEffects = false
    });
    ItemDefinitionManager::instance().registerEffect("cloakOfResistance1", ItemEffectDefinition{
        .id = "test.equipped",
        .description = "Effetto mentre equipaggiato",
        .activation = ItemEffectActivation::Equipped,
        .apply = [&equipmentApplications, &equipmentCleanups](ResourceManager &, const ItemEffectContext &context)
        {
            assert(context.itemDefinitionId == "cloakOfResistance1");
            assert(context.quantity == 1);
            ++equipmentApplications;
            return [&equipmentCleanups]
            {
                ++equipmentCleanups;
            };
        }
    });

    inventory.addItem(ItemInstanceDefinition{
        .id = "rope.slotless",
        .itemDefinitionId = "hempRope15m",
        .quantity = 1
    });
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("rope.slotless");
    }));

    inventory.addItem(ItemInstanceDefinition{
        .id = "cloak.home",
        .itemDefinitionId = "cloakOfResistance1",
        .quantity = 1
    }, "home");
    assert(encumbrance.toView().totalWeightGrams == 5000);
    assert(equipmentApplications == 0);
    inventory.equip("cloak.home");
    assert(encumbrance.toView().totalWeightGrams == 5500);
    assert(equipmentApplications == 1);
    assert(equipmentCleanups == 0);
    InventoryView inventoryView = inventory.toView();
    const auto equippedCloak = std::ranges::find(inventoryView.items, "cloak.home", &InventoryItemView::id);
    assert(equippedCloak != inventoryView.items.end());
    assert(equippedCloak->equipped);
    assert(equippedCloak->containerId == MainContainerId);
    const auto shoulderSlot = std::ranges::find(inventoryView.equipmentSlots, EquipmentSlot::Shoulders, &EquipmentSlotView::slot);
    assert(shoulderSlot != inventoryView.equipmentSlots.end());
    assert((shoulderSlot->itemIds == std::vector<std::string>{"cloak.home"}));
    const InventorySaveData inventoryData = inventory.toSaveData();
    const auto equippedCloakData = std::ranges::find(inventoryData.items, "cloak.home", &InventoryItemSaveData::id);
    assert(equippedCloakData != inventoryData.items.end());
    assert(equippedCloakData->equipped);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("cloak.home", "home");
    }));
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeItem("cloak.home");
    }));
    inventory.equip("cloak.home");
    assert(equipmentApplications == 1);
    inventory.unequip("cloak.home", "home");
    assert(encumbrance.toView().totalWeightGrams == 5000);
    assert(equipmentCleanups == 1);

    inventory.addItem(ItemInstanceDefinition{
        .id = "cloak.main",
        .itemDefinitionId = "cloakOfResistance1",
        .quantity = 1
    });
    inventory.equip("cloak.main");
    assert(equipmentApplications == 2);
    assert(encumbrance.toView().totalWeightGrams == 5500);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("cloak.home");
    }));
    assert(equipmentApplications == 2);
    assert(encumbrance.toView().totalWeightGrams == 5500);
    inventory.unequip("cloak.main");
    assert(equipmentCleanups == 2);
    assert(encumbrance.toView().totalWeightGrams == 5500);

    inventory.equip("cloak.home");
    assert(equipmentApplications == 3);
    assert(encumbrance.toView().totalWeightGrams == 6000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("cloak.home", "home");
    }));
    inventory.unequip("cloak.home", "home");
    assert(equipmentCleanups == 3);
    assert(encumbrance.toView().totalWeightGrams == 5500);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.unequip("cloak.home");
    }));

    inventory.removeItem("cloak.home");
    inventory.removeItem("cloak.main");
    inventory.removeItem("rope.slotless");
    assert(encumbrance.toView().totalWeightGrams == 0);

    inventory.addItem(ItemInstanceDefinition{.id = "ring.1", .itemDefinitionId = "ringOfProtection1", .quantity = 1});
    inventory.addItem(ItemInstanceDefinition{.id = "ring.2", .itemDefinitionId = "ringOfProtection1", .quantity = 1});
    inventory.addItem(ItemInstanceDefinition{.id = "ring.3", .itemDefinitionId = "ringOfProtection1", .quantity = 1});
    inventory.equip("ring.1");
    inventory.equip("ring.2");
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("ring.3");
    }));
    inventory.unequip("ring.1");
    inventory.equip("ring.3");
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeItem("ring.2");
    }));
    inventory.unequip("ring.2");
    inventory.removeItem("ring.2");
    inventory.unequip("ring.3");
    inventory.removeItem("ring.1");
    inventory.removeItem("ring.3");

    inventory.addItem(ItemInstanceDefinition{.id = "ring.stack", .itemDefinitionId = "ringOfProtection1", .quantity = 2});
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("ring.stack");
    }));
    inventory.removeItem("ring.stack");

    int rollbackApplications = 0;
    int rollbackCleanups = 0;
    ItemDefinitionManager::instance().registerEffect("ringOfProtection1", ItemEffectDefinition{
        .id = "test.rollback.first",
        .description = "Primo effetto transazionale",
        .activation = ItemEffectActivation::Equipped,
        .apply = [&rollbackApplications, &rollbackCleanups](ResourceManager &, const ItemEffectContext &)
        {
            ++rollbackApplications;
            return [&rollbackCleanups]
            {
                ++rollbackCleanups;
            };
        }
    });
    ItemDefinitionManager::instance().registerEffect("ringOfProtection1", ItemEffectDefinition{
        .id = "test.rollback.invalid",
        .description = "Cleanup non valido",
        .activation = ItemEffectActivation::Equipped,
        .apply = [&rollbackApplications](ResourceManager &, const ItemEffectContext &)
        {
            ++rollbackApplications;
            return ItemEffectCleanup{};
        }
    });
    inventory.addItem(ItemInstanceDefinition{.id = "ring.rollback", .itemDefinitionId = "ringOfProtection1", .quantity = 1});
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("ring.rollback");
    }));
    assert(rollbackApplications == 2);
    assert(rollbackCleanups == 1);
    inventory.removeItem("ring.rollback");

    return 0;
}
