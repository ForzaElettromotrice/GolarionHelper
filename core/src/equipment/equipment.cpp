#include "golarion/equipment/equipment.hpp"

#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::array EquipmentSlots{
        golarion::EquipmentSlot::Ring,
        golarion::EquipmentSlot::Armor,
        golarion::EquipmentSlot::Belt,
        golarion::EquipmentSlot::Neck,
        golarion::EquipmentSlot::Body,
        golarion::EquipmentSlot::Headband,
        golarion::EquipmentSlot::Hands,
        golarion::EquipmentSlot::Eyes,
        golarion::EquipmentSlot::Feet,
        golarion::EquipmentSlot::Wrists,
        golarion::EquipmentSlot::Shield,
        golarion::EquipmentSlot::Shoulders,
        golarion::EquipmentSlot::Head,
        golarion::EquipmentSlot::Chest
    };
}

namespace golarion
{
    Equipment::Equipment(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        for (const EquipmentSlot slot : EquipmentSlots)
        {
            auto [slotItems, inserted] = itemIdsBySlot_.try_emplace(slot);
            if (!inserted)
            {
                throw std::logic_error("duplicate equipment slot");
            }
            slotItems->second.reserve(capacity(slot));
        }
    }

    Equipment::~Equipment()
    {
        clear();
    }

    bool Equipment::isEquipped(std::string_view itemId) const
    {
        return itemSlots_.contains(std::string(itemId));
    }

    void Equipment::validateEquip(const ItemInstance &item) const
    {
        const ItemDefinition &definition = item.itemDefinition_.get();
        if (!definition.slot.has_value())
        {
            throw std::invalid_argument("item does not occupy an equipment slot: " + item.id_);
        }
        if (item.quantity_ != 1)
        {
            throw std::invalid_argument("equipped item instance must have quantity 1: " + item.id_);
        }

        const auto assignment = itemSlots_.find(item.id_);
        if (assignment != itemSlots_.end())
        {
            if (assignment->second != *definition.slot || !appliedEffects_.contains(item.id_))
            {
                throw std::logic_error("equipped item assignment is inconsistent: " + item.id_);
            }
            return;
        }
        if (appliedEffects_.contains(item.id_))
        {
            throw std::logic_error("item equipment effects exist without a slot assignment: " + item.id_);
        }

        const std::vector<std::string> &slotItems = itemIdsBySlot_.at(*definition.slot);
        if (slotItems.size() >= capacity(*definition.slot))
        {
            throw std::invalid_argument("equipment slot is full: " + std::string(displayName(*definition.slot)));
        }
    }

    void Equipment::validateUnequip(const ItemInstance &item) const
    {
        const auto assignment = itemSlots_.find(item.id_);
        if (assignment == itemSlots_.end())
        {
            throw std::invalid_argument("item instance is not equipped: " + item.id_);
        }
        if (!appliedEffects_.contains(item.id_))
        {
            throw std::logic_error("equipped item has no active equipment effects: " + item.id_);
        }

        const std::vector<std::string> &slotItems = itemIdsBySlot_.at(assignment->second);
        if (std::ranges::find(slotItems, item.id_) == slotItems.end())
        {
            throw std::logic_error("equipped item is absent from its slot: " + item.id_);
        }
    }

    void Equipment::equip(const ItemInstance &item)
    {
        validateEquip(item);
        if (isEquipped(item.id_))
        {
            return;
        }

        const EquipmentSlot slot = *item.itemDefinition_.get().slot;
        std::vector<std::string> &slotItems = itemIdsBySlot_.at(slot);
        auto [assignment, assignmentInserted] = itemSlots_.emplace(item.id_, slot);
        if (!assignmentInserted)
        {
            throw std::logic_error("validated equipment assignment unexpectedly failed: " + item.id_);
        }

        bool effectsInserted = false;
        bool slotItemInserted = false;
        try
        {
            auto [appliedEffects, inserted] = appliedEffects_.try_emplace(item.id_);
            if (!inserted)
            {
                throw std::logic_error("item equipment effects are already applied: " + item.id_);
            }
            effectsInserted = true;
            slotItems.push_back(item.id_);
            slotItemInserted = true;
            appliedEffects->second = item.applyEffects(resourceManager_, ItemEffectActivation::Equipped);
        }
        catch (...)
        {
            if (effectsInserted)
            {
                auto appliedEffects = appliedEffects_.find(item.id_);
                item.removeEffects(appliedEffects->second);
                appliedEffects_.erase(appliedEffects);
            }
            if (slotItemInserted)
            {
                slotItems.pop_back();
            }
            itemSlots_.erase(assignment);
            throw;
        }
    }

    void Equipment::unequip(const ItemInstance &item) noexcept
    {
        const auto assignment = itemSlots_.find(item.id_);
        const auto appliedEffects = appliedEffects_.find(item.id_);
        if (assignment == itemSlots_.end() || appliedEffects == appliedEffects_.end())
        {
            std::terminate();
        }

        std::vector<std::string> &slotItems = itemIdsBySlot_.at(assignment->second);
        const auto slotItem = std::ranges::find(slotItems, item.id_);
        if (slotItem == slotItems.end())
        {
            std::terminate();
        }

        item.removeEffects(appliedEffects->second);
        appliedEffects_.erase(appliedEffects);
        *slotItem = std::move(slotItems.back());
        slotItems.pop_back();
        itemSlots_.erase(assignment);
    }

    void Equipment::clear() noexcept
    {
        for (auto appliedEffects = appliedEffects_.rbegin(); appliedEffects != appliedEffects_.rend(); ++appliedEffects)
        {
            for (auto cleanup = appliedEffects->second.rbegin(); cleanup != appliedEffects->second.rend(); ++cleanup)
            {
                (*cleanup)();
            }
        }
        appliedEffects_.clear();
        itemSlots_.clear();
        for (auto &[slot, itemIds] : itemIdsBySlot_)
        {
            static_cast<void>(slot);
            itemIds.clear();
        }
    }
}
