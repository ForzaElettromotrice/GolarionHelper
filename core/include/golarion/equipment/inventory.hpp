#pragma once

#include "golarion/equipment/container.hpp"
#include "golarion/equipment/equipment.hpp"
#include "golarion/equipment/item.hpp"
#include "golarion/equipment/money.hpp"
#include "golarion/data/inventory_save_data.hpp"
#include "golarion/view/inventory_view.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view MainContainerId = "worn";
    inline constexpr std::string_view InventoryWeightId = "inventory.totalWeight";

    class ResourceManager;
    class CharacterSheet;

    class Inventory final
    {
    public:
        explicit Inventory(ResourceManager &resourceManager);
        ~Inventory();

        Inventory(const Inventory &) = delete;
        Inventory &operator=(const Inventory &) = delete;
        Inventory(Inventory &&) = delete;
        Inventory &operator=(Inventory &&) = delete;

        void addContainer(ContainerDefinition definition);
        void addItem(ItemInstanceDefinition definition, std::optional<std::string> containerId = std::nullopt);
        void equip(std::string_view itemId);
        void unequip(std::string_view itemId, std::optional<std::string> containerId = std::nullopt);
        void moveItem(std::string_view itemId, std::optional<std::string> containerId = std::nullopt);
        void removeItem(std::string_view itemId);
        void addMoney(CoinDenomination denomination, int quantity, std::string_view containerId);
        void removeMoney(CoinDenomination denomination, int quantity, std::string_view containerId);
        InventoryView toView() const;
        InventorySaveData toSaveData() const;

    private:
        friend class CharacterSheet;
        friend class Money;

        enum class EquipmentTransition
        {
            None,
            Equip,
            Unequip
        };

        void moveItemInternal(std::string_view itemId, std::optional<std::string> containerId, EquipmentTransition equipmentTransition);
        void validateInsertion(std::string_view destinationContainerId, const ItemInstance &item, std::int64_t incomingWeightGrams, std::int64_t incomingVolumeMilliliters, std::optional<std::string_view> excludedItemId) const;
        void validateMoveDestination(std::string_view itemId, std::string_view destinationContainerId) const;
        std::vector<std::string> subtreeItemIds(std::string_view itemId) const;
        void collectSubtreeItemIds(std::string_view itemId, std::vector<std::string> &itemIds) const;
        bool allowsPossessionEffects(std::string_view containerId) const;
        std::int64_t totalWeightGrams() const;
        std::int64_t effectiveWeightGrams(std::string_view itemId) const;
        std::int64_t effectiveWeightGrams(std::string_view itemId, std::optional<std::string_view> excludedItemId) const;
        std::int64_t effectiveWeightGrams(std::string_view itemId, std::vector<std::string> &activeItemIds, std::optional<std::string_view> excludedItemId) const;
        std::int64_t rootWeightContribution(std::string_view itemId) const;
        std::int64_t effectiveVolumeMilliliters(std::string_view itemId) const;
        std::int64_t effectiveVolumeMilliliters(std::string_view itemId, std::optional<std::string_view> excludedItemId) const;
        std::int64_t effectiveVolumeMilliliters(std::string_view itemId, std::vector<std::string> &activeItemIds, std::optional<std::string_view> excludedItemId) const;
        void replaceCoinQuantity(std::string_view itemId, int quantity);
        void replaceRegisteredWeight(std::int64_t previousWeightGrams, std::int64_t newWeightGrams);
        void load(const InventorySaveData &data);

        ResourceManager &resourceManager_;
        Equipment equipment_;
        std::map<std::string, ItemInstance> items_;
        std::map<std::string, Container> containers_;
        std::map<std::string, std::string> itemContainerIds_;
        std::map<std::string, std::vector<ItemEffectCleanup>> appliedPossessionEffects_;
        Money money_;
    };
}
