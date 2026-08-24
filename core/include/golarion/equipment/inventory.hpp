#pragma once

#include "golarion/equipment/item.hpp"
#include "golarion/equipment/item_location.hpp"

#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view InventoryItemsResource = "inventory.items";

    class ResourceManager;

    class Inventory final
    {
    public:
        explicit Inventory(ResourceManager &resourceManager);

        Inventory(const Inventory &) = delete;
        Inventory &operator=(const Inventory &) = delete;
        Inventory(Inventory &&) = delete;
        Inventory &operator=(Inventory &&) = delete;

    private:
        struct StoredItem
        {
            ItemInstance item;
            ItemLocation location;
        };

        void addItem(ItemInstance item);
        void removeItem(std::string_view itemId);

        ResourceManager &resourceManager_;
        std::map<std::string, StoredItem> items_;
    };
}
