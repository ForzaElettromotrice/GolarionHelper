#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct ItemSelectorSaveData
    {
        std::vector<std::string> definitionIds;
        std::vector<std::string> tags;
    };

    struct ItemQuantityLimitSaveData
    {
        std::int64_t maximumQuantity;
        std::optional<ItemSelectorSaveData> selector;
    };

    struct InventoryContainerSaveData
    {
        std::string id;
        std::string name;
        std::optional<std::int64_t> maximumContentsWeightGrams;
        std::optional<std::int64_t> maximumContentsVolumeMilliliters;
        std::optional<ItemSelectorSaveData> acceptedItems;
        std::vector<ItemQuantityLimitSaveData> quantityLimits;
        bool ignoresContentsWeight;
        bool ignoresContentsVolume;
        bool allowsPossessionEffects;
        bool contributesToCarriedWeight = false;
    };

    struct ItemChoiceSelectionSaveData
    {
        std::string choiceId;
        std::vector<std::string> optionIds;
    };

    struct InventoryItemSaveData
    {
        std::string id;
        std::string itemDefinitionId;
        int quantity;
        std::vector<ItemChoiceSelectionSaveData> choices;
        std::string containerId;
        bool equipped;
    };

    struct InventorySaveData
    {
        std::vector<InventoryContainerSaveData> containers;
        std::vector<InventoryItemSaveData> items;
    };
}
