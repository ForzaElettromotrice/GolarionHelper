#include "golarion/equipment/inventory.hpp"

#include "golarion/character/encumbrance.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view InventoryWeightSource = "Inventario";

    std::string itemContainerId(std::string_view itemId)
    {
        return "item." + std::string(itemId);
    }

    std::int64_t checkedMeasureSum(std::int64_t left, std::int64_t right, std::string_view measure)
    {
        if (right > std::numeric_limits<std::int64_t>::max() - left)
        {
            throw std::invalid_argument("inventory " + std::string(measure) + " is out of range");
        }
        return left + right;
    }

    golarion::Container containerFromItem(std::string_view itemId, const golarion::ItemDefinition &itemDefinition)
    {
        const golarion::ItemContainerDefinition &containerDefinition = *itemDefinition.container;
        return golarion::Container(golarion::ContainerDefinition{
            .id = itemContainerId(itemId),
            .name = itemDefinition.name,
            .ownerItemId = std::string(itemId),
            .maximumContentsWeightGrams = containerDefinition.maximumContentsWeightGrams,
            .maximumContentsVolumeMilliliters = containerDefinition.maximumContentsVolumeMilliliters,
            .acceptedItems = containerDefinition.acceptedItems,
            .quantityLimits = containerDefinition.quantityLimits,
            .ignoresContentsWeight = containerDefinition.ignoresContentsWeight,
            .ignoresContentsVolume = containerDefinition.ignoresContentsVolume,
            .allowsPossessionEffects = containerDefinition.allowsPossessionEffects
        });
    }
}

namespace golarion
{
    Inventory::Inventory(ResourceManager &resourceManager) : resourceManager_(resourceManager), equipment_(resourceManager), money_(*this)
    {
        containers_.emplace(std::string(MainContainerId), Container(ContainerDefinition{
            .id = std::string(MainContainerId),
            .name = "Indossati",
            .ownerItemId = std::nullopt,
            .maximumContentsWeightGrams = std::nullopt,
            .maximumContentsVolumeMilliliters = std::nullopt,
            .acceptedItems = std::nullopt,
            .quantityLimits = {},
            .ignoresContentsWeight = false,
            .ignoresContentsVolume = false,
            .allowsPossessionEffects = true,
            .contributesToCarriedWeight = true
        }));
        resourceManager_.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
            .id = std::string(InventoryWeightId),
            .source = std::string(InventoryWeightSource),
            .grams = 0
        }));
    }

    Inventory::~Inventory()
    {
        equipment_.clear();
        for (auto appliedEffects = appliedPossessionEffects_.rbegin(); appliedEffects != appliedPossessionEffects_.rend(); ++appliedEffects)
        {
            const auto item = items_.find(appliedEffects->first);
            if (item == items_.end())
            {
                std::terminate();
            }
            item->second.removeEffects(appliedEffects->second);
        }
    }

    void Inventory::addContainer(ContainerDefinition definition)
    {
        Container container(std::move(definition));
        if (container.ownerItemId_.has_value())
        {
            throw std::invalid_argument("virtual container must not reference an owner item");
        }
        if (container.id_.starts_with("item."))
        {
            throw std::invalid_argument("virtual container ID uses a reserved container namespace: " + container.id_);
        }

        const std::string containerId = container.id_;
        if (!containers_.emplace(containerId, std::move(container)).second)
        {
            throw std::invalid_argument("container is already registered: " + containerId);
        }
    }

    void Inventory::addItem(ItemInstanceDefinition definition, std::optional<std::string> containerId)
    {
        const std::string destinationContainerId = containerId.has_value() ? normalize(*containerId) : std::string(MainContainerId);
        auto destination = containers_.find(destinationContainerId);
        if (destination == containers_.end())
        {
            throw std::invalid_argument("destination container is not registered: " + destinationContainerId);
        }
        ItemInstance item(std::move(definition));
        const std::string itemId = item.id_;
        if (items_.contains(itemId))
        {
            throw std::invalid_argument("item instance is already registered: " + itemId);
        }

        const ItemDefinition &itemDefinition = item.itemDefinition_.get();
        std::optional<Container> itemContainer;
        std::string generatedContainerId;
        if (itemDefinition.container.has_value())
        {
            if (item.quantity_ != 1)
            {
                throw std::invalid_argument("container item instances must have quantity 1");
            }
            generatedContainerId = itemContainerId(itemId);
            if (containers_.contains(generatedContainerId))
            {
                throw std::invalid_argument("item container is already registered: " + generatedContainerId);
            }
            itemContainer.emplace(containerFromItem(itemId, itemDefinition));
        }

        validateInsertion(destinationContainerId, item, item.weightGrams(), item.volumeMilliliters(), std::nullopt);
        const std::int64_t previousWeightGrams = totalWeightGrams();
        bool itemInserted = false;
        bool containerInserted = false;
        bool membershipInserted = false;
        bool locationInserted = false;
        bool weightReplaced = false;
        bool appliedEffectsInserted = false;
        std::int64_t newWeightGrams = previousWeightGrams;
        try
        {
            itemInserted = items_.emplace(itemId, std::move(item)).second;
            if (!itemInserted)
            {
                throw std::logic_error("validated item insertion unexpectedly failed");
            }
            if (itemContainer.has_value())
            {
                containerInserted = containers_.emplace(generatedContainerId, std::move(*itemContainer)).second;
                if (!containerInserted)
                {
                    throw std::logic_error("validated item container insertion unexpectedly failed");
                }
            }
            destination->second.itemIds_.push_back(itemId);
            membershipInserted = true;
            locationInserted = itemContainerIds_.emplace(itemId, destinationContainerId).second;
            if (!locationInserted)
            {
                throw std::logic_error("validated item location insertion unexpectedly failed");
            }
            newWeightGrams = totalWeightGrams();
            replaceRegisteredWeight(previousWeightGrams, newWeightGrams);
            weightReplaced = previousWeightGrams != newWeightGrams;

            if (allowsPossessionEffects(destinationContainerId))
            {
                auto [appliedEffects, inserted] = appliedPossessionEffects_.try_emplace(itemId);
                if (!inserted)
                {
                    throw std::logic_error("item possession effects are already applied: " + itemId);
                }
                appliedEffectsInserted = true;
                appliedEffects->second = items_.at(itemId).applyEffects(resourceManager_, ItemEffectActivation::Possessed);
            }
        }
        catch (...)
        {
            if (appliedEffectsInserted)
            {
                auto appliedEffects = appliedPossessionEffects_.find(itemId);
                items_.at(itemId).removeEffects(appliedEffects->second);
                appliedPossessionEffects_.erase(appliedEffects);
            }
            if (weightReplaced)
            {
                try
                {
                    replaceRegisteredWeight(newWeightGrams, previousWeightGrams);
                }
                catch (...)
                {
                    std::terminate();
                }
            }
            if (locationInserted)
            {
                itemContainerIds_.erase(itemId);
            }
            if (membershipInserted)
            {
                destination->second.itemIds_.pop_back();
            }
            if (containerInserted)
            {
                containers_.erase(generatedContainerId);
            }
            if (itemInserted)
            {
                items_.erase(itemId);
            }
            throw;
        }
    }

    void Inventory::equip(std::string_view itemId)
    {
        const std::string id = normalize(itemId);
        const auto item = items_.find(id);
        if (item == items_.end())
        {
            throw std::invalid_argument("item instance is not registered: " + id);
        }

        equipment_.validateEquip(item->second);
        if (equipment_.isEquipped(id))
        {
            if (itemContainerIds_.at(id) != MainContainerId)
            {
                throw std::logic_error("equipped item is outside the main worn container: " + id);
            }
            return;
        }
        moveItemInternal(id, std::nullopt, EquipmentTransition::Equip);
    }

    void Inventory::unequip(std::string_view itemId, std::optional<std::string> containerId)
    {
        moveItemInternal(itemId, std::move(containerId), EquipmentTransition::Unequip);
    }

    void Inventory::moveItem(std::string_view itemId, std::optional<std::string> containerId)
    {
        moveItemInternal(itemId, std::move(containerId), EquipmentTransition::None);
    }

    void Inventory::moveItemInternal(std::string_view itemId, std::optional<std::string> containerId, EquipmentTransition equipmentTransition)
    {
        const std::string id = normalize(itemId);
        const auto item = items_.find(id);
        if (item == items_.end())
        {
            throw std::invalid_argument("item instance is not registered: " + id);
        }
        const std::string destinationContainerId = containerId.has_value() ? normalize(*containerId) : std::string(MainContainerId);
        auto destination = containers_.find(destinationContainerId);
        if (destination == containers_.end())
        {
            throw std::invalid_argument("destination container is not registered: " + destinationContainerId);
        }
        const bool itemIsEquipped = equipment_.isEquipped(id);
        if (equipmentTransition == EquipmentTransition::None && itemIsEquipped)
        {
            throw std::invalid_argument("equipped item must be moved through Inventory::unequip(): " + id);
        }
        if (equipmentTransition == EquipmentTransition::Equip && itemIsEquipped)
        {
            throw std::logic_error("item is already equipped: " + id);
        }
        if (equipmentTransition == EquipmentTransition::Unequip && !itemIsEquipped)
        {
            throw std::invalid_argument("item instance is not equipped: " + id);
        }
        if (equipmentTransition == EquipmentTransition::Equip && destinationContainerId != MainContainerId)
        {
            throw std::logic_error("equipped item destination must be the main worn container");
        }
        if (equipmentTransition == EquipmentTransition::Equip)
        {
            equipment_.validateEquip(item->second);
        }
        else if (equipmentTransition == EquipmentTransition::Unequip)
        {
            equipment_.validateUnequip(item->second);
        }

        auto location = itemContainerIds_.find(id);
        if (location == itemContainerIds_.end())
        {
            throw std::logic_error("item instance has no inventory location: " + id);
        }
        auto source = containers_.find(location->second);
        if (source == containers_.end())
        {
            throw std::logic_error("item location references an absent container: " + location->second);
        }
        const auto membership = std::ranges::find(source->second.itemIds_, id);
        if (membership == source->second.itemIds_.end())
        {
            throw std::logic_error("item location is missing from its container: " + id);
        }
        if (source == destination && equipmentTransition == EquipmentTransition::None)
        {
            return;
        }

        validateMoveDestination(id, destinationContainerId);
        const std::vector<std::string> movedItemIds = subtreeItemIds(id);
        std::vector<bool> oldPossessionEffectStates;
        oldPossessionEffectStates.reserve(movedItemIds.size());
        for (const std::string &movedItemId : movedItemIds)
        {
            const std::string &currentContainerId = itemContainerIds_.at(movedItemId);
            const bool possessionEffectsAllowed = allowsPossessionEffects(currentContainerId);
            if (appliedPossessionEffects_.contains(movedItemId) != possessionEffectsAllowed)
            {
                throw std::logic_error("item possession effect state does not match its location: " + movedItemId);
            }
            oldPossessionEffectStates.push_back(possessionEffectsAllowed);
        }

        if (source != destination)
        {
            const std::int64_t incomingWeightGrams = effectiveWeightGrams(id);
            const std::int64_t incomingVolumeMilliliters = effectiveVolumeMilliliters(id);
            validateInsertion(destinationContainerId, item->second, incomingWeightGrams, incomingVolumeMilliliters, id);
        }

        const std::int64_t previousWeightGrams = totalWeightGrams();
        const std::size_t sourceIndex = static_cast<std::size_t>(std::distance(source->second.itemIds_.begin(), membership));
        if (source != destination)
        {
            destination->second.itemIds_.reserve(destination->second.itemIds_.size() + 1);
        }
        std::string movedMembership = id;
        std::string swappedLocation = destinationContainerId;
        std::vector<std::size_t> possessionEffectsToActivate;
        std::vector<std::size_t> possessionEffectsToDeactivate;
        possessionEffectsToActivate.reserve(movedItemIds.size());
        possessionEffectsToDeactivate.reserve(movedItemIds.size());

        bool hierarchyMoved = false;
        bool weightReplaced = false;
        std::size_t insertedPossessionEffectStates = 0;
        bool equipmentApplied = false;
        std::int64_t newWeightGrams = previousWeightGrams;
        try
        {
            if (source != destination)
            {
                destination->second.itemIds_.push_back(std::move(movedMembership));
                source->second.itemIds_.erase(source->second.itemIds_.begin() + static_cast<std::ptrdiff_t>(sourceIndex));
                location->second.swap(swappedLocation);
                hierarchyMoved = true;
            }

            for (std::size_t index = 0; index < movedItemIds.size(); ++index)
            {
                const std::string &newContainerId = itemContainerIds_.at(movedItemIds[index]);
                const bool newPossessionEffectState = allowsPossessionEffects(newContainerId);
                if (!oldPossessionEffectStates[index] && newPossessionEffectState)
                {
                    possessionEffectsToActivate.push_back(index);
                }
                else if (oldPossessionEffectStates[index] && !newPossessionEffectState)
                {
                    possessionEffectsToDeactivate.push_back(index);
                }
            }

            for (const std::size_t index : possessionEffectsToActivate)
            {
                if (!appliedPossessionEffects_.try_emplace(movedItemIds[index]).second)
                {
                    throw std::logic_error("item possession effects are already applied: " + movedItemIds[index]);
                }
                ++insertedPossessionEffectStates;
            }
            newWeightGrams = totalWeightGrams();
            replaceRegisteredWeight(previousWeightGrams, newWeightGrams);
            weightReplaced = previousWeightGrams != newWeightGrams;

            for (const std::size_t index : possessionEffectsToActivate)
            {
                const std::string &activatedItemId = movedItemIds[index];
                appliedPossessionEffects_.at(activatedItemId) = items_.at(activatedItemId).applyEffects(resourceManager_, ItemEffectActivation::Possessed);
            }
            if (equipmentTransition == EquipmentTransition::Equip)
            {
                equipment_.equip(item->second);
                equipmentApplied = true;
            }

            if (equipmentTransition == EquipmentTransition::Unequip)
            {
                equipment_.unequip(item->second);
            }
            for (auto index = possessionEffectsToDeactivate.rbegin(); index != possessionEffectsToDeactivate.rend(); ++index)
            {
                const std::string &deactivatedItemId = movedItemIds[*index];
                auto appliedEffects = appliedPossessionEffects_.find(deactivatedItemId);
                items_.at(deactivatedItemId).removeEffects(appliedEffects->second);
                appliedPossessionEffects_.erase(appliedEffects);
            }
        }
        catch (...)
        {
            if (equipmentApplied)
            {
                equipment_.unequip(item->second);
            }
            for (std::size_t offset = insertedPossessionEffectStates; offset > 0; --offset)
            {
                const std::string &activatedItemId = movedItemIds[possessionEffectsToActivate[offset - 1]];
                auto appliedEffects = appliedPossessionEffects_.find(activatedItemId);
                items_.at(activatedItemId).removeEffects(appliedEffects->second);
                appliedPossessionEffects_.erase(appliedEffects);
            }
            if (weightReplaced)
            {
                try
                {
                    replaceRegisteredWeight(newWeightGrams, previousWeightGrams);
                }
                catch (...)
                {
                    std::terminate();
                }
            }
            if (hierarchyMoved)
            {
                try
                {
                    location->second.swap(swappedLocation);
                    std::string restoredMembership = std::move(destination->second.itemIds_.back());
                    destination->second.itemIds_.pop_back();
                    source->second.itemIds_.insert(source->second.itemIds_.begin() + static_cast<std::ptrdiff_t>(sourceIndex), std::move(restoredMembership));
                }
                catch (...)
                {
                    std::terminate();
                }
            }
            throw;
        }
    }

    void Inventory::removeItem(std::string_view itemId)
    {
        const std::string id = normalize(itemId);
        const auto item = items_.find(id);
        if (item == items_.end())
        {
            throw std::invalid_argument("item instance is not registered: " + id);
        }
        if (equipment_.isEquipped(id))
        {
            throw std::invalid_argument("equipped item must be unequipped before removal: " + id);
        }

        const auto location = itemContainerIds_.find(id);
        if (location == itemContainerIds_.end())
        {
            throw std::logic_error("item instance has no inventory location: " + id);
        }
        auto containingContainer = containers_.find(location->second);
        if (containingContainer == containers_.end())
        {
            throw std::logic_error("item location references an absent container: " + location->second);
        }
        const auto membership = std::ranges::find(containingContainer->second.itemIds_, id);
        if (membership == containingContainer->second.itemIds_.end())
        {
            throw std::logic_error("item location is missing from its container: " + id);
        }

        const std::string generatedContainerId = itemContainerId(id);
        const auto generatedContainer = containers_.find(generatedContainerId);
        if (item->second.itemDefinition_.get().container.has_value())
        {
            if (generatedContainer == containers_.end())
            {
                throw std::logic_error("container item has no generated container: " + id);
            }
            if (!generatedContainer->second.itemIds_.empty())
            {
                throw std::invalid_argument("container item is not empty: " + id);
            }
        }
        else if (generatedContainer != containers_.end())
        {
            throw std::logic_error("non-container item unexpectedly owns a container: " + id);
        }

        const std::int64_t previousWeightGrams = totalWeightGrams();
        const std::int64_t removedWeightGrams = rootWeightContribution(id);
        if (removedWeightGrams > previousWeightGrams)
        {
            throw std::logic_error("item root weight contribution exceeds inventory total: " + id);
        }
        replaceRegisteredWeight(previousWeightGrams, previousWeightGrams - removedWeightGrams);

        const auto appliedEffects = appliedPossessionEffects_.find(id);
        if (appliedEffects != appliedPossessionEffects_.end())
        {
            item->second.removeEffects(appliedEffects->second);
            appliedPossessionEffects_.erase(appliedEffects);
        }

        containingContainer->second.itemIds_.erase(membership);
        itemContainerIds_.erase(location);
        if (generatedContainer != containers_.end())
        {
            containers_.erase(generatedContainer);
        }
        items_.erase(item);
    }

    void Inventory::addMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        money_.add(denomination, quantity, containerId);
    }

    void Inventory::removeMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        money_.remove(denomination, quantity, containerId);
    }

    InventoryView Inventory::toView() const
    {
        const auto selectorView = [](const ItemSelector &selector)
        {
            return ItemSelectorView{
                .definitionIds = selector.definitionIds_,
                .tags = selector.tags_
            };
        };

        std::vector<InventoryItemView> itemViews;
        itemViews.reserve(items_.size());
        for (const auto &[id, item] : items_)
        {
            const ItemDefinition &definition = item.itemDefinition_.get();
            std::vector<ItemChoiceView> choiceViews;
            choiceViews.reserve(definition.choices.size());
            for (std::size_t choiceIndex = 0; choiceIndex < definition.choices.size(); ++choiceIndex)
            {
                const ItemChoiceDefinition &choice = definition.choices[choiceIndex];
                const ItemChoiceSelection &selection = item.choices_[choiceIndex];
                std::vector<ItemChoiceOptionView> optionViews;
                optionViews.reserve(choice.options.size());
                for (const ItemChoiceOptionDefinition &option : choice.options)
                {
                    optionViews.push_back(ItemChoiceOptionView{
                        .id = option.id,
                        .name = option.name,
                        .selected = std::ranges::find(selection.optionIds, option.id) != selection.optionIds.end()
                    });
                }
                choiceViews.push_back(ItemChoiceView{
                    .id = choice.id,
                    .prompt = choice.prompt,
                    .selectionCount = choice.selectionCount,
                    .options = std::move(optionViews)
                });
            }
            itemViews.push_back(InventoryItemView{
                .id = id,
                .itemDefinitionId = definition.id,
                .name = definition.name,
                .tags = definition.tags,
                .quantity = item.quantity_,
                .intrinsicWeightGrams = item.weightGrams(),
                .intrinsicVolumeMilliliters = item.volumeMilliliters(),
                .effectiveWeightGrams = effectiveWeightGrams(id),
                .effectiveVolumeMilliliters = effectiveVolumeMilliliters(id),
                .equipmentSlot = definition.slot,
                .choices = std::move(choiceViews),
                .containerId = itemContainerIds_.at(id),
                .ownedContainerId = definition.container.has_value() ? std::optional<std::string>(itemContainerId(id)) : std::nullopt,
                .equipped = equipment_.isEquipped(id),
                .possessionEffectsActive = appliedPossessionEffects_.contains(id)
            });
        }

        std::vector<InventoryContainerView> containerViews;
        containerViews.reserve(containers_.size());
        for (const auto &[id, container] : containers_)
        {
            std::vector<ItemQuantityLimitView> quantityLimitViews;
            quantityLimitViews.reserve(container.quantityLimits_.size());
            for (const ItemQuantityLimit &limit : container.quantityLimits_)
            {
                quantityLimitViews.push_back(ItemQuantityLimitView{
                    .maximumQuantity = limit.maximumQuantity,
                    .selector = limit.selector.has_value() ? std::optional<ItemSelectorView>(selectorView(*limit.selector)) : std::nullopt
                });
            }

            std::int64_t currentContentsWeightGrams = 0;
            std::int64_t currentContentsVolumeMilliliters = 0;
            for (const std::string &itemId : container.itemIds_)
            {
                currentContentsWeightGrams = checkedMeasureSum(currentContentsWeightGrams, effectiveWeightGrams(itemId), "weight");
                currentContentsVolumeMilliliters = checkedMeasureSum(currentContentsVolumeMilliliters, effectiveVolumeMilliliters(itemId), "volume");
            }

            containerViews.push_back(InventoryContainerView{
                .id = id,
                .name = container.name_,
                .ownerItemId = container.ownerItemId_,
                .maximumContentsWeightGrams = container.maximumContentsWeightGrams_,
                .maximumContentsVolumeMilliliters = container.maximumContentsVolumeMilliliters_,
                .acceptedItems = container.acceptedItems_.has_value() ? std::optional<ItemSelectorView>(selectorView(*container.acceptedItems_)) : std::nullopt,
                .quantityLimits = std::move(quantityLimitViews),
                .ignoresContentsWeight = container.ignoresContentsWeight_,
                .ignoresContentsVolume = container.ignoresContentsVolume_,
                .allowsPossessionEffects = container.allowsPossessionEffects_,
                .contributesToCarriedWeight = container.contributesToCarriedWeight_,
                .currentContentsWeightGrams = currentContentsWeightGrams,
                .currentContentsVolumeMilliliters = currentContentsVolumeMilliliters,
                .itemIds = container.itemIds_
            });
        }

        std::vector<EquipmentSlotView> equipmentSlotViews;
        equipmentSlotViews.reserve(equipment_.itemIdsBySlot_.size());
        for (const auto &[slot, itemIds] : equipment_.itemIdsBySlot_)
        {
            equipmentSlotViews.push_back(EquipmentSlotView{
                .slot = slot,
                .capacity = golarion::capacity(slot),
                .itemIds = itemIds
            });
        }

        return InventoryView{
            .totalCarriedWeightGrams = totalWeightGrams(),
            .items = std::move(itemViews),
            .containers = std::move(containerViews),
            .equipmentSlots = std::move(equipmentSlotViews),
            .money = money_.toView()
        };
    }

    InventorySaveData Inventory::toSaveData() const
    {
        const auto selectorData = [](const ItemSelector &selector)
        {
            return ItemSelectorSaveData{
                .definitionIds = selector.definitionIds_,
                .tags = selector.tags_
            };
        };

        std::vector<InventoryContainerSaveData> containerData;
        for (const auto &[id, container] : containers_)
        {
            if (id == MainContainerId || container.ownerItemId_.has_value())
            {
                continue;
            }

            std::vector<ItemQuantityLimitSaveData> quantityLimits;
            quantityLimits.reserve(container.quantityLimits_.size());
            for (const ItemQuantityLimit &limit : container.quantityLimits_)
            {
                quantityLimits.push_back(ItemQuantityLimitSaveData{
                    .maximumQuantity = limit.maximumQuantity,
                    .selector = limit.selector.has_value() ? std::optional<ItemSelectorSaveData>(selectorData(*limit.selector)) : std::nullopt
                });
            }

            containerData.push_back(InventoryContainerSaveData{
                .id = id,
                .name = container.name_,
                .maximumContentsWeightGrams = container.maximumContentsWeightGrams_,
                .maximumContentsVolumeMilliliters = container.maximumContentsVolumeMilliliters_,
                .acceptedItems = container.acceptedItems_.has_value() ? std::optional<ItemSelectorSaveData>(selectorData(*container.acceptedItems_)) : std::nullopt,
                .quantityLimits = std::move(quantityLimits),
                .ignoresContentsWeight = container.ignoresContentsWeight_,
                .ignoresContentsVolume = container.ignoresContentsVolume_,
                .allowsPossessionEffects = container.allowsPossessionEffects_,
                .contributesToCarriedWeight = container.contributesToCarriedWeight_
            });
        }

        std::vector<InventoryItemSaveData> itemData;
        itemData.reserve(items_.size());
        for (const auto &[id, item] : items_)
        {
            std::vector<ItemChoiceSelectionSaveData> choices;
            choices.reserve(item.choices_.size());
            for (const ItemChoiceSelection &choice : item.choices_)
            {
                choices.push_back(ItemChoiceSelectionSaveData{
                    .choiceId = choice.choiceId,
                    .optionIds = choice.optionIds
                });
            }
            itemData.push_back(InventoryItemSaveData{
                .id = id,
                .itemDefinitionId = item.itemDefinition_.get().id,
                .quantity = item.quantity_,
                .choices = std::move(choices),
                .containerId = itemContainerIds_.at(id),
                .equipped = equipment_.isEquipped(id)
            });
        }

        return InventorySaveData{
            .containers = std::move(containerData),
            .items = std::move(itemData)
        };
    }

    void Inventory::load(const InventorySaveData &data)
    {
        if (!items_.empty() || containers_.size() != 1 || !containers_.contains(std::string(MainContainerId)))
        {
            throw std::logic_error("inventory can only be loaded into its initial empty state");
        }

        const auto selector = [](const ItemSelectorSaveData &selectorData)
        {
            return ItemSelector(ItemSelectorDefinition{
                .definitionIds = selectorData.definitionIds,
                .tags = selectorData.tags
            });
        };

        for (const InventoryContainerSaveData &container : data.containers)
        {
            std::vector<ItemQuantityLimit> quantityLimits;
            quantityLimits.reserve(container.quantityLimits.size());
            for (const ItemQuantityLimitSaveData &limit : container.quantityLimits)
            {
                quantityLimits.push_back(ItemQuantityLimit{
                    .maximumQuantity = limit.maximumQuantity,
                    .selector = limit.selector.has_value() ? std::optional<ItemSelector>(selector(*limit.selector)) : std::nullopt
                });
            }
            addContainer(ContainerDefinition{
                .id = container.id,
                .name = container.name,
                .ownerItemId = std::nullopt,
                .maximumContentsWeightGrams = container.maximumContentsWeightGrams,
                .maximumContentsVolumeMilliliters = container.maximumContentsVolumeMilliliters,
                .acceptedItems = container.acceptedItems.has_value() ? std::optional<ItemSelector>(selector(*container.acceptedItems)) : std::nullopt,
                .quantityLimits = std::move(quantityLimits),
                .ignoresContentsWeight = container.ignoresContentsWeight,
                .ignoresContentsVolume = container.ignoresContentsVolume,
                .allowsPossessionEffects = container.allowsPossessionEffects,
                .contributesToCarriedWeight = container.contributesToCarriedWeight
            });
        }

        std::vector<InventoryItemSaveData> pendingItems = data.items;
        while (!pendingItems.empty())
        {
            bool addedItem = false;
            for (auto item = pendingItems.begin(); item != pendingItems.end();)
            {
                if (!containers_.contains(normalize(item->containerId)))
                {
                    ++item;
                    continue;
                }
                if (item->equipped && normalize(item->containerId) != MainContainerId)
                {
                    throw std::invalid_argument("equipped inventory item must be stored in the worn container: " + item->id);
                }
                addItem(ItemInstanceDefinition{
                    .id = item->id,
                    .itemDefinitionId = item->itemDefinitionId,
                    .quantity = item->quantity,
                    .choices = [&item]
                    {
                        std::vector<ItemChoiceSelection> choices;
                        choices.reserve(item->choices.size());
                        for (const ItemChoiceSelectionSaveData &choice : item->choices)
                        {
                            choices.push_back(ItemChoiceSelection{
                                .choiceId = choice.choiceId,
                                .optionIds = choice.optionIds
                            });
                        }
                        return choices;
                    }()
                }, item->containerId);
                item = pendingItems.erase(item);
                addedItem = true;
            }
            if (!addedItem)
            {
                throw std::invalid_argument("inventory items reference missing or cyclic containers");
            }
        }

        for (const InventoryItemSaveData &item : data.items)
        {
            if (item.equipped)
            {
                equip(item.id);
            }
        }
    }

    void Inventory::validateInsertion(std::string_view destinationContainerId, const ItemInstance &item, std::int64_t incomingWeightGrams, std::int64_t incomingVolumeMilliliters, std::optional<std::string_view> excludedItemId) const
    {
        const ContainerResolvers resolvers{
            .itemWeight = [this, excludedItemId](std::string_view containedItemId)
            {
                return effectiveWeightGrams(containedItemId, excludedItemId);
            },
            .itemVolume = [this, excludedItemId](std::string_view containedItemId)
            {
                return effectiveVolumeMilliliters(containedItemId, excludedItemId);
            },
            .itemQuantity = [this, excludedItemId](std::string_view containedItemId, const ItemSelector &selector)
            {
                if (excludedItemId.has_value() && containedItemId == *excludedItemId)
                {
                    return std::int64_t{0};
                }
                return items_.at(std::string(containedItemId)).matchingQuantity(selector);
            }
        };

        const Container *container = &containers_.at(std::string(destinationContainerId));
        container->validateItem(item, incomingWeightGrams, incomingVolumeMilliliters, resolvers);
        while (container->ownerItemId_.has_value())
        {
            if (container->ignoresContentsWeight_)
            {
                incomingWeightGrams = 0;
            }
            if (container->ignoresContentsVolume_)
            {
                incomingVolumeMilliliters = 0;
            }
            if (incomingWeightGrams == 0 && incomingVolumeMilliliters == 0)
            {
                break;
            }

            const auto ownerLocation = itemContainerIds_.find(*container->ownerItemId_);
            if (ownerLocation == itemContainerIds_.end())
            {
                throw std::logic_error("container owner has no inventory location: " + *container->ownerItemId_);
            }
            container = &containers_.at(ownerLocation->second);
            container->validateAdditionalContents(incomingWeightGrams, incomingVolumeMilliliters, resolvers);
        }
    }

    void Inventory::validateMoveDestination(std::string_view itemId, std::string_view destinationContainerId) const
    {
        std::vector<std::string> visitedContainerIds;
        const Container *container = &containers_.at(std::string(destinationContainerId));
        while (true)
        {
            if (std::ranges::find(visitedContainerIds, container->id_) != visitedContainerIds.end())
            {
                throw std::logic_error("inventory container hierarchy contains a cycle at container: " + container->id_);
            }
            visitedContainerIds.push_back(container->id_);
            if (!container->ownerItemId_.has_value())
            {
                return;
            }
            if (*container->ownerItemId_ == itemId)
            {
                throw std::invalid_argument("item cannot be moved inside its own container subtree: " + std::string(itemId));
            }

            const auto ownerLocation = itemContainerIds_.find(*container->ownerItemId_);
            if (ownerLocation == itemContainerIds_.end())
            {
                throw std::logic_error("container owner has no inventory location: " + *container->ownerItemId_);
            }
            container = &containers_.at(ownerLocation->second);
        }
    }

    std::vector<std::string> Inventory::subtreeItemIds(std::string_view itemId) const
    {
        std::vector<std::string> itemIds;
        collectSubtreeItemIds(itemId, itemIds);
        return itemIds;
    }

    void Inventory::collectSubtreeItemIds(std::string_view itemId, std::vector<std::string> &itemIds) const
    {
        const std::string id(itemId);
        if (std::ranges::find(itemIds, id) != itemIds.end())
        {
            throw std::logic_error("inventory item hierarchy contains a duplicate or cycle at item: " + id);
        }
        if (!items_.contains(id))
        {
            throw std::logic_error("inventory container references an absent item: " + id);
        }
        itemIds.push_back(id);

        const auto ownedContainer = containers_.find(itemContainerId(id));
        if (ownedContainer == containers_.end())
        {
            return;
        }
        for (const std::string &containedItemId : ownedContainer->second.itemIds_)
        {
            collectSubtreeItemIds(containedItemId, itemIds);
        }
    }

    bool Inventory::allowsPossessionEffects(std::string_view containerId) const
    {
        const Container *container = &containers_.at(std::string(containerId));
        while (true)
        {
            if (!container->allowsPossessionEffects_)
            {
                return false;
            }
            if (!container->ownerItemId_.has_value())
            {
                return true;
            }

            const auto ownerLocation = itemContainerIds_.find(*container->ownerItemId_);
            if (ownerLocation == itemContainerIds_.end())
            {
                throw std::logic_error("container owner has no inventory location: " + *container->ownerItemId_);
            }
            container = &containers_.at(ownerLocation->second);
        }
    }

    std::int64_t Inventory::totalWeightGrams() const
    {
        std::int64_t total = 0;
        for (const auto &[containerId, container] : containers_)
        {
            static_cast<void>(containerId);
            if (container.ownerItemId_.has_value() || !container.contributesToCarriedWeight_)
            {
                continue;
            }
            for (const std::string &itemId : container.itemIds_)
            {
                total = checkedMeasureSum(total, effectiveWeightGrams(itemId), "weight");
            }
        }
        return total;
    }

    std::int64_t Inventory::effectiveWeightGrams(std::string_view itemId) const
    {
        return effectiveWeightGrams(itemId, std::nullopt);
    }

    std::int64_t Inventory::effectiveWeightGrams(std::string_view itemId, std::optional<std::string_view> excludedItemId) const
    {
        std::vector<std::string> activeItemIds;
        return effectiveWeightGrams(itemId, activeItemIds, excludedItemId);
    }

    std::int64_t Inventory::effectiveWeightGrams(std::string_view itemId, std::vector<std::string> &activeItemIds, std::optional<std::string_view> excludedItemId) const
    {
        const std::string id(itemId);
        if (excludedItemId.has_value() && id == *excludedItemId)
        {
            return 0;
        }
        if (std::ranges::find(activeItemIds, id) != activeItemIds.end())
        {
            throw std::logic_error("inventory container hierarchy contains a cycle at item: " + id);
        }
        activeItemIds.push_back(id);

        const ItemInstance &item = items_.at(id);
        std::int64_t total = item.weightGrams();
        const auto ownedContainer = containers_.find(itemContainerId(id));
        if (ownedContainer != containers_.end() && !ownedContainer->second.ignoresContentsWeight_)
        {
            for (const std::string &containedItemId : ownedContainer->second.itemIds_)
            {
                total = checkedMeasureSum(total, effectiveWeightGrams(containedItemId, activeItemIds, excludedItemId), "weight");
            }
        }
        activeItemIds.pop_back();
        return total;
    }

    std::int64_t Inventory::rootWeightContribution(std::string_view itemId) const
    {
        const std::string id(itemId);
        const auto location = itemContainerIds_.find(id);
        if (location == itemContainerIds_.end())
        {
            throw std::logic_error("item instance has no inventory location: " + id);
        }

        std::string containerId = location->second;
        while (true)
        {
            const Container &container = containers_.at(containerId);
            if (container.ignoresContentsWeight_)
            {
                return 0;
            }
            if (!container.ownerItemId_.has_value())
            {
                return container.contributesToCarriedWeight_ ? effectiveWeightGrams(id) : 0;
            }

            const auto ownerLocation = itemContainerIds_.find(*container.ownerItemId_);
            if (ownerLocation == itemContainerIds_.end())
            {
                throw std::logic_error("container owner has no inventory location: " + *container.ownerItemId_);
            }
            containerId = ownerLocation->second;
        }
    }

    std::int64_t Inventory::effectiveVolumeMilliliters(std::string_view itemId) const
    {
        return effectiveVolumeMilliliters(itemId, std::nullopt);
    }

    std::int64_t Inventory::effectiveVolumeMilliliters(std::string_view itemId, std::optional<std::string_view> excludedItemId) const
    {
        std::vector<std::string> activeItemIds;
        return effectiveVolumeMilliliters(itemId, activeItemIds, excludedItemId);
    }

    std::int64_t Inventory::effectiveVolumeMilliliters(std::string_view itemId, std::vector<std::string> &activeItemIds, std::optional<std::string_view> excludedItemId) const
    {
        const std::string id(itemId);
        if (excludedItemId.has_value() && id == *excludedItemId)
        {
            return 0;
        }
        if (std::ranges::find(activeItemIds, id) != activeItemIds.end())
        {
            throw std::logic_error("inventory container hierarchy contains a cycle at item: " + id);
        }
        activeItemIds.push_back(id);

        const ItemInstance &item = items_.at(id);
        std::int64_t total = item.volumeMilliliters();
        const auto ownedContainer = containers_.find(itemContainerId(id));
        if (ownedContainer != containers_.end() && !ownedContainer->second.ignoresContentsVolume_)
        {
            for (const std::string &containedItemId : ownedContainer->second.itemIds_)
            {
                total = checkedMeasureSum(total, effectiveVolumeMilliliters(containedItemId, activeItemIds, excludedItemId), "volume");
            }
        }
        activeItemIds.pop_back();
        return total;
    }

    void Inventory::replaceCoinQuantity(std::string_view itemId, int quantity)
    {
        if (quantity < 1)
        {
            throw std::invalid_argument("coin quantity must be at least 1");
        }

        const std::string id = normalize(itemId);
        auto item = items_.find(id);
        if (item == items_.end())
        {
            throw std::invalid_argument("coin item instance is not registered: " + id);
        }
        const ItemDefinition &definition = item->second.itemDefinition_.get();
        if (!definition.coinDenomination.has_value())
        {
            throw std::invalid_argument("item instance is not a coin: " + id);
        }
        if (item->second.quantity_ == quantity)
        {
            return;
        }

        const ItemInstance replacement(ItemInstanceDefinition{
            .id = id,
            .itemDefinitionId = definition.id,
            .quantity = quantity
        });
        const std::string &containerId = itemContainerIds_.at(id);
        validateInsertion(containerId, replacement, replacement.weightGrams(), replacement.volumeMilliliters(), id);

        const std::int64_t previousWeightGrams = totalWeightGrams();
        const int previousQuantity = item->second.quantity_;
        item->second.quantity_ = quantity;
        const std::int64_t newWeightGrams = totalWeightGrams();
        try
        {
            replaceRegisteredWeight(previousWeightGrams, newWeightGrams);
        }
        catch (...)
        {
            item->second.quantity_ = previousQuantity;
            throw;
        }
    }

    void Inventory::replaceRegisteredWeight(std::int64_t previousWeightGrams, std::int64_t newWeightGrams)
    {
        if (previousWeightGrams == newWeightGrams)
        {
            return;
        }

        resourceManager_.removeFromCollection(CarriedWeightsResource, InventoryWeightId);
        try
        {
            resourceManager_.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
                .id = std::string(InventoryWeightId),
                .source = std::string(InventoryWeightSource),
                .grams = newWeightGrams
            }));
        }
        catch (...)
        {
            try
            {
                resourceManager_.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
                    .id = std::string(InventoryWeightId),
                    .source = std::string(InventoryWeightSource),
                    .grams = previousWeightGrams
                }));
            }
            catch (...)
            {
                std::terminate();
            }
            throw;
        }
    }
}
