#pragma once

#include "golarion/equipment/coin.hpp"
#include "golarion/equipment/equipment_slot.hpp"
#include "golarion/effect/effect_compiler.hpp"

#include <cstddef>
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

    struct ItemEffectDefinition
    {
        ItemEffectActivation activation;
        EffectDefinition effect;
    };

    struct ItemChoiceOptionDefinition
    {
        std::string id;
        std::string name;
        std::vector<ItemEffectDefinition> effects{};
    };

    struct ItemChoiceDefinition
    {
        std::string id;
        std::string prompt;
        std::size_t selectionCount;
        std::vector<ItemChoiceOptionDefinition> options;
    };

    struct ItemChoiceSelection
    {
        std::string choiceId;
        std::vector<std::string> optionIds;
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
        std::vector<ItemChoiceDefinition> choices;
    };

    struct ItemInstanceDefinition
    {
        std::string id;
        std::string itemDefinitionId;
        int quantity = 1;
        std::vector<ItemChoiceSelection> choices{};
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

        struct CompiledEffect
        {
            std::string id;
            EffectApply apply;
        };

        std::vector<EffectCleanup> applyEffects(ResourceManager &resourceManager, ItemEffectActivation activation) const;
        void removeEffects(std::vector<EffectCleanup> &cleanups) const noexcept;

        std::string id_;
        std::reference_wrapper<const ItemDefinition> itemDefinition_;
        int quantity_;
        std::vector<ItemChoiceSelection> choices_;
        std::vector<CompiledEffect> possessedEffects_;
        std::vector<CompiledEffect> equippedEffects_;
    };
}
