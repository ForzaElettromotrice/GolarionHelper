#pragma once

#include "golarion/equipment/equipment_slot.hpp"
#include "golarion/view/money_view.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct ItemSelectorView
    {
        std::vector<std::string> definitionIds;
        std::vector<std::string> tags;
    };

    struct ItemQuantityLimitView
    {
        std::int64_t maximumQuantity;
        std::optional<ItemSelectorView> selector;
    };

    struct InventoryItemView
    {
        std::string id;
        std::string itemDefinitionId;
        std::string name;
        std::vector<std::string> tags;
        int quantity;
        std::int64_t intrinsicWeightGrams;
        std::int64_t intrinsicVolumeMilliliters;
        std::int64_t effectiveWeightGrams;
        std::int64_t effectiveVolumeMilliliters;
        std::optional<EquipmentSlot> equipmentSlot;
        std::string containerId;
        std::optional<std::string> ownedContainerId;
        bool equipped;
        bool possessionEffectsActive;
    };

    struct InventoryContainerView
    {
        std::string id;
        std::string name;
        std::optional<std::string> ownerItemId;
        std::optional<std::int64_t> maximumContentsWeightGrams;
        std::optional<std::int64_t> maximumContentsVolumeMilliliters;
        std::optional<ItemSelectorView> acceptedItems;
        std::vector<ItemQuantityLimitView> quantityLimits;
        bool ignoresContentsWeight;
        bool ignoresContentsVolume;
        bool allowsPossessionEffects;
        bool contributesToCarriedWeight;
        std::int64_t currentContentsWeightGrams;
        std::int64_t currentContentsVolumeMilliliters;
        std::vector<std::string> itemIds;
    };

    struct EquipmentSlotView
    {
        EquipmentSlot slot;
        std::size_t capacity;
        std::vector<std::string> itemIds;
    };

    struct InventoryView
    {
        std::int64_t totalCarriedWeightGrams;
        std::vector<InventoryItemView> items;
        std::vector<InventoryContainerView> containers;
        std::vector<EquipmentSlotView> equipmentSlots;
        MoneyView money;
    };
}
