#include "golarion/equipment/container.hpp"

#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    Container::Container(ContainerDefinition definition)
        : id_(normalize(definition.id)),
          name_(normalize(definition.name)),
          ownerItemId_(std::move(definition.ownerItemId)),
          maximumContentsWeightGrams_(definition.maximumContentsWeightGrams),
          maximumContentsVolumeMilliliters_(definition.maximumContentsVolumeMilliliters),
          acceptedItems_(std::move(definition.acceptedItems)),
          quantityLimits_(std::move(definition.quantityLimits)),
          ignoresContentsWeight_(definition.ignoresContentsWeight),
          ignoresContentsVolume_(definition.ignoresContentsVolume),
          allowsPossessionEffects_(definition.allowsPossessionEffects),
          contributesToCarriedWeight_(definition.contributesToCarriedWeight)
    {
        if (ownerItemId_.has_value())
        {
            ownerItemId_ = normalize(*ownerItemId_);
            if (contributesToCarriedWeight_)
            {
                throw std::invalid_argument("item-owned container cannot be a carried-weight root");
            }
        }
        if (maximumContentsWeightGrams_.has_value() && *maximumContentsWeightGrams_ < 0)
        {
            throw std::invalid_argument("container maximum contents weight must not be negative");
        }
        if (maximumContentsVolumeMilliliters_.has_value() && *maximumContentsVolumeMilliliters_ < 0)
        {
            throw std::invalid_argument("container maximum contents volume must not be negative");
        }
        for (const ItemQuantityLimit &quantityLimit : quantityLimits_)
        {
            if (quantityLimit.maximumQuantity < 0)
            {
                throw std::invalid_argument("container maximum item quantity must not be negative");
            }
        }
    }

    void Container::validateItem(const ItemInstance &item, std::int64_t effectiveWeightGrams, std::int64_t effectiveVolumeMilliliters, const ContainerResolvers &resolvers) const
    {
        if (!canAcceptQuantity(item, resolvers.itemQuantity))
        {
            throw std::invalid_argument("item is not accepted by container rules: " + id_);
        }
        validateAdditionalContents(effectiveWeightGrams, effectiveVolumeMilliliters, resolvers);
    }

    void Container::validateAdditionalContents(std::int64_t weightGrams, std::int64_t volumeMilliliters, const ContainerResolvers &resolvers) const
    {
        if (!canAcceptWeight(weightGrams, resolvers.itemWeight))
        {
            throw std::invalid_argument("contents exceed container weight limit: " + id_);
        }
        if (!canAcceptVolume(volumeMilliliters, resolvers.itemVolume))
        {
            throw std::invalid_argument("contents exceed container volume limit: " + id_);
        }
    }

    bool Container::canAcceptWeight(std::int64_t incomingWeightGrams, const std::function<std::int64_t(std::string_view)> &itemWeightResolver) const
    {
        if (incomingWeightGrams < 0)
        {
            throw std::invalid_argument("incoming item weight must not be negative");
        }
        if (!maximumContentsWeightGrams_.has_value())
        {
            return true;
        }
        if (!itemWeightResolver)
        {
            throw std::invalid_argument("item weight resolver must be provided for a weight-limited container");
        }

        std::int64_t currentWeightGrams = 0;
        for (const std::string &itemId : itemIds_)
        {
            const std::int64_t itemWeightGrams = itemWeightResolver(itemId);
            if (itemWeightGrams < 0)
            {
                throw std::invalid_argument("resolved item weight must not be negative");
            }
            if (itemWeightGrams > *maximumContentsWeightGrams_ - currentWeightGrams)
            {
                return false;
            }
            currentWeightGrams += itemWeightGrams;
        }
        return incomingWeightGrams <= *maximumContentsWeightGrams_ - currentWeightGrams;
    }

    bool Container::canAcceptVolume(std::int64_t incomingVolumeMilliliters, const std::function<std::int64_t(std::string_view)> &itemVolumeResolver) const
    {
        if (incomingVolumeMilliliters < 0)
        {
            throw std::invalid_argument("incoming item volume must not be negative");
        }
        if (!maximumContentsVolumeMilliliters_.has_value())
        {
            return true;
        }
        if (!itemVolumeResolver)
        {
            throw std::invalid_argument("item volume resolver must be provided for a volume-limited container");
        }

        std::int64_t currentVolumeMilliliters = 0;
        for (const std::string &itemId : itemIds_)
        {
            const std::int64_t itemVolumeMilliliters = itemVolumeResolver(itemId);
            if (itemVolumeMilliliters < 0)
            {
                throw std::invalid_argument("resolved item volume must not be negative");
            }
            if (itemVolumeMilliliters > *maximumContentsVolumeMilliliters_ - currentVolumeMilliliters)
            {
                return false;
            }
            currentVolumeMilliliters += itemVolumeMilliliters;
        }
        return incomingVolumeMilliliters <= *maximumContentsVolumeMilliliters_ - currentVolumeMilliliters;
    }

    bool Container::canAcceptQuantity(const ItemInstance &incomingItem, const std::function<std::int64_t(std::string_view, const ItemSelector &)> &itemQuantityResolver) const
    {
        if (acceptedItems_.has_value() && !incomingItem.matches(*acceptedItems_))
        {
            return false;
        }

        for (const ItemQuantityLimit &quantityLimit : quantityLimits_)
        {
            const ItemSelector allItems(ItemSelectorDefinition{});
            const ItemSelector &selector = quantityLimit.selector.has_value() ? *quantityLimit.selector : allItems;
            const std::int64_t incomingQuantity = incomingItem.matchingQuantity(selector);
            if (incomingQuantity == 0)
            {
                continue;
            }
            if (!itemQuantityResolver)
            {
                throw std::invalid_argument("item quantity resolver must be provided for a quantity-limited container");
            }

            std::int64_t currentQuantity = 0;
            for (const std::string &itemId : itemIds_)
            {
                const std::int64_t itemQuantity = itemQuantityResolver(itemId, selector);
                if (itemQuantity < 0)
                {
                    throw std::invalid_argument("resolved item quantity must not be negative");
                }
                if (itemQuantity > quantityLimit.maximumQuantity - currentQuantity)
                {
                    return false;
                }
                currentQuantity += itemQuantity;
            }
            if (incomingQuantity > quantityLimit.maximumQuantity - currentQuantity)
            {
                return false;
            }
        }
        return true;
    }
}
