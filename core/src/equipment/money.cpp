#include "golarion/equipment/money.hpp"

#include "golarion/equipment/inventory.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::int64_t checkedAdd(std::int64_t left, std::int64_t right, std::string_view description)
    {
        if (right < 0 || right > std::numeric_limits<std::int64_t>::max() - left)
        {
            throw std::invalid_argument(std::string(description) + " is out of range");
        }
        return left + right;
    }

    std::int64_t &amount(golarion::CoinAmountsView &amounts, golarion::CoinDenomination denomination)
    {
        switch (denomination)
        {
            case golarion::CoinDenomination::Copper:
                return amounts.copper;
            case golarion::CoinDenomination::Silver:
                return amounts.silver;
            case golarion::CoinDenomination::Gold:
                return amounts.gold;
            case golarion::CoinDenomination::Platinum:
                return amounts.platinum;
        }
        throw std::invalid_argument("unknown coin denomination");
    }

    void addAmounts(golarion::CoinAmountsView &destination, const golarion::CoinAmountsView &source)
    {
        destination.copper = checkedAdd(destination.copper, source.copper, "copper coin total");
        destination.silver = checkedAdd(destination.silver, source.silver, "silver coin total");
        destination.gold = checkedAdd(destination.gold, source.gold, "gold coin total");
        destination.platinum = checkedAdd(destination.platinum, source.platinum, "platinum coin total");
        destination.weightGrams = checkedAdd(destination.weightGrams, source.weightGrams, "coin weight total");
    }
}

namespace golarion
{
    Money::Money(Inventory &inventory) : inventory_(inventory)
    {
    }

    void Money::add(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        if (quantity <= 0)
        {
            throw std::invalid_argument("coin quantity to add must be positive");
        }

        const std::string normalizedContainerId = normalize(containerId);
        const auto container = inventory_.containers_.find(normalizedContainerId);
        if (container == inventory_.containers_.end())
        {
            throw std::invalid_argument("destination container is not registered: " + normalizedContainerId);
        }

        for (const std::string &itemId : container->second.itemIds_)
        {
            const ItemInstance &item = inventory_.items_.at(itemId);
            if (item.itemDefinition_.get().coinDenomination != denomination)
            {
                continue;
            }
            if (quantity > std::numeric_limits<int>::max() - item.quantity_)
            {
                throw std::invalid_argument("coin quantity is out of range");
            }
            inventory_.replaceCoinQuantity(itemId, item.quantity_ + quantity);
            return;
        }

        const std::string definitionId(coinItemDefinitionId(denomination));
        const std::string baseItemId = "money." + normalizedContainerId + "." + definitionId;
        std::string itemId = baseItemId;
        for (std::size_t suffix = 1; inventory_.items_.contains(itemId); ++suffix)
        {
            itemId = baseItemId + "." + std::to_string(suffix);
        }
        inventory_.addItem(ItemInstanceDefinition{
            .id = std::move(itemId),
            .itemDefinitionId = definitionId,
            .quantity = quantity
        }, normalizedContainerId);
    }

    void Money::remove(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        if (quantity <= 0)
        {
            throw std::invalid_argument("coin quantity to remove must be positive");
        }

        const std::string normalizedContainerId = normalize(containerId);
        const auto container = inventory_.containers_.find(normalizedContainerId);
        if (container == inventory_.containers_.end())
        {
            throw std::invalid_argument("source container is not registered: " + normalizedContainerId);
        }

        struct QuantityChange
        {
            std::string itemId;
            std::string itemDefinitionId;
            int previousQuantity;
            int newQuantity;
        };

        int remaining = quantity;
        std::vector<QuantityChange> changes;
        for (const std::string &itemId : container->second.itemIds_)
        {
            const ItemInstance &item = inventory_.items_.at(itemId);
            if (item.itemDefinition_.get().coinDenomination != denomination)
            {
                continue;
            }

            const int removed = std::min(remaining, item.quantity_);
            changes.push_back(QuantityChange{
                .itemId = itemId,
                .itemDefinitionId = item.itemDefinition_.get().id,
                .previousQuantity = item.quantity_,
                .newQuantity = item.quantity_ - removed
            });
            remaining -= removed;
            if (remaining == 0)
            {
                break;
            }
        }
        if (remaining != 0)
        {
            throw std::invalid_argument("container does not contain enough coins of the requested denomination: " + normalizedContainerId);
        }

        std::size_t appliedChanges = 0;
        try
        {
            for (const QuantityChange &change : changes)
            {
                if (change.newQuantity == 0)
                {
                    inventory_.removeItem(change.itemId);
                }
                else
                {
                    inventory_.replaceCoinQuantity(change.itemId, change.newQuantity);
                }
                ++appliedChanges;
            }
        }
        catch (...)
        {
            while (appliedChanges > 0)
            {
                --appliedChanges;
                const QuantityChange &change = changes[appliedChanges];
                try
                {
                    if (change.newQuantity == 0)
                    {
                        inventory_.addItem(ItemInstanceDefinition{
                            .id = change.itemId,
                            .itemDefinitionId = change.itemDefinitionId,
                            .quantity = change.previousQuantity
                        }, normalizedContainerId);
                    }
                    else
                    {
                        inventory_.replaceCoinQuantity(change.itemId, change.previousQuantity);
                    }
                }
                catch (...)
                {
                    std::terminate();
                }
            }
            throw;
        }
    }

    MoneyView Money::toView() const
    {
        MoneyView view{
            .containers = {},
            .total = {}
        };

        for (const auto &[containerId, container] : inventory_.containers_)
        {
            CoinAmountsView amounts{};
            bool containsCoins = false;
            for (const std::string &itemId : container.itemIds_)
            {
                const ItemInstance &item = inventory_.items_.at(itemId);
                const std::optional<CoinDenomination> denomination = item.itemDefinition_.get().coinDenomination;
                if (!denomination.has_value())
                {
                    continue;
                }

                std::int64_t &denominationAmount = amount(amounts, *denomination);
                denominationAmount = checkedAdd(denominationAmount, item.quantity_, "coin quantity");
                amounts.weightGrams = checkedAdd(amounts.weightGrams, item.weightGrams(), "coin weight");
                containsCoins = true;
            }
            if (!containsCoins)
            {
                continue;
            }

            addAmounts(view.total, amounts);
            view.containers.push_back(ContainerMoneyView{
                .containerId = containerId,
                .containerName = container.name_,
                .amounts = std::move(amounts)
            });
        }
        return view;
    }
}
