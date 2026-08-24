#include "golarion/equipment/inventory.hpp"

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/encumbrance_view.hpp"
#include "golarion/view/resource_manager_view.hpp"

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
        return 10;
    });
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);
    Inventory inventory(resourceManager);

    const ResourceManagerView managerView = resourceManager.toView();
    assert(std::ranges::find(managerView.collections, InventoryItemsResource) != managerView.collections.end());

    resourceManager.addToCollection(InventoryItemsResource, ItemInstance(ItemInstanceDefinition{
        .id = "rope.1",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    }));
    EncumbranceView encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 10000);
    assert(encumbranceView.weights.size() == 1);
    assert(encumbranceView.weights[0].id == "item.rope.1");

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(InventoryItemsResource, ItemInstance(ItemInstanceDefinition{
            .id = "rope.1",
            .itemDefinitionId = "hempRope15m",
            .quantity = 1
        }));
    }));
    assert(encumbrance.toView().totalWeightGrams == 10000);
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(InventoryItemsResource, std::string("wrong type"));
    }));

    resourceManager.removeFromCollection(InventoryItemsResource, " rope.1 ");
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 0);
    assert(encumbranceView.weights.empty());
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(InventoryItemsResource, "rope.1");
    }));

    return 0;
}
