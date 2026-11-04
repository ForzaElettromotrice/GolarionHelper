#pragma once

#include "golarion/equipment/item.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class Inventory;
    class Money;

    struct ContainerResolvers
    {
        std::function<std::int64_t(std::string_view)> itemWeight;
        std::function<std::int64_t(std::string_view)> itemVolume;
        std::function<std::int64_t(std::string_view, const ItemSelector &)> itemQuantity;
    };

    struct ContainerDefinition
    {
        std::string id;
        std::string name;
        std::optional<std::string> ownerItemId;
        std::optional<std::int64_t> maximumContentsWeightGrams;
        std::optional<std::int64_t> maximumContentsVolumeMilliliters;
        std::optional<ItemSelector> acceptedItems;
        std::vector<ItemQuantityLimit> quantityLimits;
        bool ignoresContentsWeight = false;
        bool ignoresContentsVolume = false;
        bool allowsPossessionEffects = true;
        bool contributesToCarriedWeight = false;
    };

    class Container final
    {
    public:
        explicit Container(ContainerDefinition definition);

        void validateItem(const ItemInstance &item, std::int64_t effectiveWeightGrams, std::int64_t effectiveVolumeMilliliters, const ContainerResolvers &resolvers) const;
        void validateAdditionalContents(std::int64_t weightGrams, std::int64_t volumeMilliliters, const ContainerResolvers &resolvers) const;

    private:
        friend class Inventory;
        friend class Money;

        bool canAcceptWeight(std::int64_t incomingWeightGrams, const std::function<std::int64_t(std::string_view)> &itemWeightResolver) const;
        bool canAcceptVolume(std::int64_t incomingVolumeMilliliters, const std::function<std::int64_t(std::string_view)> &itemVolumeResolver) const;
        bool canAcceptQuantity(const ItemInstance &incomingItem, const std::function<std::int64_t(std::string_view, const ItemSelector &)> &itemQuantityResolver) const;

        std::string id_;
        std::string name_;
        std::optional<std::string> ownerItemId_;
        std::optional<std::int64_t> maximumContentsWeightGrams_;
        std::optional<std::int64_t> maximumContentsVolumeMilliliters_;
        std::optional<ItemSelector> acceptedItems_;
        std::vector<ItemQuantityLimit> quantityLimits_;
        bool ignoresContentsWeight_;
        bool ignoresContentsVolume_;
        bool allowsPossessionEffects_;
        bool contributesToCarriedWeight_;
        std::vector<std::string> itemIds_;
    };
}
