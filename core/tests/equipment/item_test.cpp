#include "golarion/equipment/item.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/encumbrance_view.hpp"

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

    const ItemInstance rope(ItemInstanceDefinition{
        .id = "rope.1",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    });
    rope.registerWeight(resourceManager);
    EncumbranceView encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 10000);
    assert(encumbranceView.weights.size() == 1);
    assert(encumbranceView.weights[0].id == "item.rope.1");
    assert(encumbranceView.weights[0].source == "Corda di canapa (15 m)");
    assert(encumbranceView.weights[0].grams == 10000);
    const ItemInstance updatedRope(ItemInstanceDefinition{
        .id = "rope.1",
        .itemDefinitionId = "hempRope15m",
        .quantity = 3
    });
    updatedRope.refreshWeight(resourceManager);
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 15000);
    assert(encumbranceView.weights.size() == 1);
    assert(encumbranceView.weights[0].grams == 15000);
    assert(throwsInvalidArgument([&]
    {
        rope.registerWeight(resourceManager);
    }));
    rope.unregisterWeight(resourceManager);
    encumbranceView = encumbrance.toView();
    assert(encumbranceView.totalWeightGrams == 0);
    assert(encumbranceView.weights.empty());
    assert(throwsInvalidArgument([&]
    {
        rope.unregisterWeight(resourceManager);
    }));

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "invalid.definition",
            .itemDefinitionId = "missing",
            .quantity = 1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "invalid.quantity",
            .itemDefinitionId = "hempRope15m",
            .quantity = 0
        }));
    }));

    return 0;
}
