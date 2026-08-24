#include "golarion/equipment/item_location.hpp"

#include <cassert>
#include <stdexcept>
#include <variant>

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

    const ItemLocation carried = CarriedItemLocation{};
    assert(std::holds_alternative<CarriedItemLocation>(carried));

    const ItemLocation contained = ContainerItemLocation(" backpack.1 ");
    assert(std::holds_alternative<ContainerItemLocation>(contained));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ContainerItemLocation("  "));
    }));

    const ItemLocation worn = EquippedItemLocation(EquippedItemLocationDefinition{
        .magicItemSlot = MagicItemSlot::Belt,
        .occupiedHands = {}
    });
    assert(std::holds_alternative<EquippedItemLocation>(worn));

    const ItemLocation wielded = EquippedItemLocation(EquippedItemLocationDefinition{
        .magicItemSlot = std::nullopt,
        .occupiedHands = {HandSlot::Main, HandSlot::Off}
    });
    assert(std::holds_alternative<EquippedItemLocation>(wielded));

    const ItemLocation shield = EquippedItemLocation(EquippedItemLocationDefinition{
        .magicItemSlot = MagicItemSlot::Shield,
        .occupiedHands = {HandSlot::Off}
    });
    assert(std::holds_alternative<EquippedItemLocation>(shield));

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(EquippedItemLocation(EquippedItemLocationDefinition{
            .magicItemSlot = std::nullopt,
            .occupiedHands = {}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(EquippedItemLocation(EquippedItemLocationDefinition{
            .magicItemSlot = std::nullopt,
            .occupiedHands = {HandSlot::Main, HandSlot::Main}
        }));
    }));

    return 0;
}
