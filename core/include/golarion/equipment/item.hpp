#pragma once

#include "golarion/equipment/coin.hpp"
#include "golarion/equipment/equipment_slot.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class Equipment;
    class Inventory;
    class Money;
    class ResourceManager;

    enum class ItemEffectActivation
    {
        Possessed,
        Equipped
    };

    std::string_view displayName(ItemEffectActivation activation);

    struct ItemEffectContext
    {
        std::string instanceId;
        std::string itemInstanceId;
        std::string itemDefinitionId;
        std::string effectId;
        std::string source;
        int quantity;
    };

    // Cleanup callbacks must not throw: once an external resource has been changed,
    // an arbitrary callback cannot be rolled back safely by Inventory.
    using ItemEffectCleanup = std::function<void()>;
    using ItemEffectApply = std::function<ItemEffectCleanup(ResourceManager &, const ItemEffectContext &)>;

    struct ItemEffectDefinition
    {
        std::string id;
        std::string description;
        ItemEffectActivation activation;
        ItemEffectApply apply;
    };

    struct ItemSelectorDefinition
    {
        std::vector<std::string> definitionIds;
        std::vector<std::string> tags;
    };

    class ItemSelector final
    {
    public:
        explicit ItemSelector(ItemSelectorDefinition definition);

    private:
        friend class ItemInstance;
        friend class Inventory;

        std::vector<std::string> definitionIds_;
        std::vector<std::string> tags_;
    };

    struct ItemQuantityLimit
    {
        std::int64_t maximumQuantity;
        std::optional<ItemSelector> selector;
    };

    struct ItemContainerDefinition
    {
        std::optional<std::int64_t> maximumContentsWeightGrams;
        std::optional<std::int64_t> maximumContentsVolumeMilliliters;
        std::optional<ItemSelector> acceptedItems;
        std::vector<ItemQuantityLimit> quantityLimits;
        bool ignoresContentsWeight = false;
        bool ignoresContentsVolume = false;
        bool allowsPossessionEffects = true;
    };

    struct ItemDefinition
    {
        std::string id;
        std::string name;
        std::int64_t weightGrams;
        std::int64_t volumeMilliliters;
        std::vector<std::string> tags;
        std::optional<CoinDenomination> coinDenomination = std::nullopt;
        std::optional<EquipmentSlot> slot;
        std::optional<ItemContainerDefinition> container;
        std::vector<ItemEffectDefinition> effects;
    };

    struct ItemInstanceDefinition
    {
        std::string id;
        std::string itemDefinitionId;
        int quantity = 1;
    };

    class ItemInstance final
    {
    public:
        explicit ItemInstance(ItemInstanceDefinition definition);

        std::int64_t weightGrams() const;
        std::int64_t volumeMilliliters() const;
        bool matches(const ItemSelector &selector) const;
        std::int64_t matchingQuantity(const ItemSelector &selector) const;

    private:
        friend class Equipment;
        friend class Inventory;
        friend class Money;

        std::vector<ItemEffectCleanup> applyEffects(ResourceManager &resourceManager, ItemEffectActivation activation) const;
        void removeEffects(std::vector<ItemEffectCleanup> &cleanups) const noexcept;

        std::string id_;
        std::reference_wrapper<const ItemDefinition> itemDefinition_;
        int quantity_;
    };
}
