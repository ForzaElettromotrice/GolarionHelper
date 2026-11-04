#pragma once

#include "golarion/equipment/equipment_slot.hpp"
#include "golarion/equipment/item.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class Inventory;
    class ResourceManager;

    class Equipment final
    {
    private:
        friend class Inventory;

        explicit Equipment(ResourceManager &resourceManager);
        ~Equipment();

        Equipment(const Equipment &) = delete;
        Equipment &operator=(const Equipment &) = delete;
        Equipment(Equipment &&) = delete;
        Equipment &operator=(Equipment &&) = delete;

        bool isEquipped(std::string_view itemId) const;
        void validateEquip(const ItemInstance &item) const;
        void validateUnequip(const ItemInstance &item) const;
        void equip(const ItemInstance &item);
        void unequip(const ItemInstance &item) noexcept;
        void clear() noexcept;

        ResourceManager &resourceManager_;
        std::map<EquipmentSlot, std::vector<std::string>> itemIdsBySlot_;
        std::map<std::string, EquipmentSlot> itemSlots_;
        std::map<std::string, std::vector<EffectCleanup>> appliedEffects_;
    };
}
