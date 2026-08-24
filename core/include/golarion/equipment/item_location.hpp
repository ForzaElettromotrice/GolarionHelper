#pragma once

#include "golarion/equipment/equipment_slot.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace golarion
{
    class Equipment;
    class Inventory;

    struct CarriedItemLocation final
    {
    };

    class ContainerItemLocation final
    {
    public:
        explicit ContainerItemLocation(std::string_view containerItemId);

    private:
        friend class Equipment;
        friend class Inventory;

        std::string containerItemId_;
    };

    struct EquippedItemLocationDefinition
    {
        std::optional<MagicItemSlot> magicItemSlot;
        std::vector<HandSlot> occupiedHands;
    };

    class EquippedItemLocation final
    {
    public:
        explicit EquippedItemLocation(EquippedItemLocationDefinition definition);

    private:
        friend class Equipment;
        friend class Inventory;

        std::optional<MagicItemSlot> magicItemSlot_;
        std::vector<HandSlot> occupiedHands_;
    };

    using ItemLocation = std::variant<CarriedItemLocation, ContainerItemLocation, EquippedItemLocation>;
}
