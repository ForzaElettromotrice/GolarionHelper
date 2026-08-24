#include "golarion/equipment/inventory.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    Inventory::Inventory(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerCollectionResource<ItemInstance>(InventoryItemsResource, [this](ItemInstance item)
        {
            addItem(std::move(item));
        }, [this](std::string_view itemId)
        {
            removeItem(itemId);
        });
    }

    void Inventory::addItem(ItemInstance item)
    {
        const std::string id = item.id_;
        auto [stored, inserted] = items_.emplace(id, StoredItem{
            .item = std::move(item),
            .location = CarriedItemLocation{}
        });
        if (!inserted)
        {
            throw std::invalid_argument("inventory item is already registered: " + id);
        }

        try
        {
            stored->second.item.registerWeight(resourceManager_);
        }
        catch (...)
        {
            items_.erase(stored);
            throw;
        }
    }

    void Inventory::removeItem(std::string_view itemId)
    {
        const std::string id = normalize(itemId);
        const auto item = items_.find(id);
        if (item == items_.end())
        {
            throw std::invalid_argument("inventory item is not registered: " + id);
        }

        item->second.item.unregisterWeight(resourceManager_);
        items_.erase(item);
    }
}
