#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/equipment/inventory.hpp"
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

    ResourceManager resourceManager;
    resourceManager.registerTarget("str", []
    {
        return 10;
    });
    resourceManager.registerEnhanceableResource("str");
    resourceManager.registerEnhanceableResource("savingThrow.all");
    resourceManager.registerEnhanceableResource("armorClass.all");
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);
    Inventory inventory(resourceManager);
    inventory.addContainer(ContainerDefinition{
        .id = "home",
        .name = "Casa",
        .allowsPossessionEffects = false
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
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    inventory.equip("cloak.home");
    assert(encumbrance.toView().totalWeightGrams == 5500);
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
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
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    inventory.unequip("cloak.home", "home");
    assert(encumbrance.toView().totalWeightGrams == 5000);
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);

    inventory.addItem(ItemInstanceDefinition{
        .id = "cloak.main",
        .itemDefinitionId = "cloakOfResistance1",
        .quantity = 1
    });
    inventory.equip("cloak.main");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 5500);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("cloak.home");
    }));
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 5500);
    inventory.unequip("cloak.main");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
    assert(encumbrance.toView().totalWeightGrams == 5500);

    inventory.equip("cloak.home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 1);
    assert(encumbrance.toView().totalWeightGrams == 6000);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.moveItem("cloak.home", "home");
    }));
    inventory.unequip("cloak.home", "home");
    assert(resourceManager.modifierTotal("savingThrow.all") == 0);
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
    assert(resourceManager.modifierTotal("armorClass.all") == 1);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("ring.3");
    }));
    inventory.unequip("ring.1");
    inventory.equip("ring.3");
    assert(resourceManager.modifierTotal("armorClass.all") == 1);
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.removeItem("ring.2");
    }));
    inventory.unequip("ring.2");
    inventory.removeItem("ring.2");
    inventory.unequip("ring.3");
    inventory.removeItem("ring.1");
    inventory.removeItem("ring.3");
    assert(resourceManager.modifierTotal("armorClass.all") == 0);

    assert(throwsInvalidArgument([&inventory]
    {
        inventory.addItem(ItemInstanceDefinition{.id = "ring.stack", .itemDefinitionId = "ringOfProtection1", .quantity = 2});
    }));

    inventory.addItem(ItemInstanceDefinition{
        .id = "belt.rollback",
        .itemDefinitionId = "beltOfPhysicalMight2",
        .quantity = 1,
        .choices = {ItemChoiceSelection{.choiceId = "abilities", .optionIds = {"strength", "dexterity"}}}
    });
    assert(throwsInvalidArgument([&inventory]
    {
        inventory.equip("belt.rollback");
    }));
    assert(resourceManager.modifierTotal("str") == 0);
    assert(!inventory.toView().items.back().equipped);

    resourceManager.registerEnhanceableResource("dex");
    inventory.equip("belt.rollback");
    assert(resourceManager.modifierTotal("str") == 2);
    assert(resourceManager.modifierTotal("dex") == 2);
    const InventoryItemView &beltView = inventory.toView().items.back();
    assert(beltView.choices.size() == 1);
    assert(beltView.choices[0].options.size() == 3);
    assert(beltView.choices[0].options[0].selected);
    assert(beltView.choices[0].options[1].selected);
    assert(!beltView.choices[0].options[2].selected);
    inventory.unequip("belt.rollback");
    assert(resourceManager.modifierTotal("str") == 0);
    assert(resourceManager.modifierTotal("dex") == 0);
    inventory.removeItem("belt.rollback");

    return 0;
}
