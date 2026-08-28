#include "golarion/equipment/item.hpp"

#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    std::string itemEffectInstanceId(std::string_view itemInstanceId, std::string_view effectId)
    {
        const auto appendPart = [](std::string &result, std::string_view part)
        {
            result += std::to_string(part.size()) + ":" + std::string(part);
        };

        std::string result = "item.effect.";
        appendPart(result, itemInstanceId);
        appendPart(result, effectId);
        return result;
    }

    void normalizeSelectorValues(std::vector<std::string> &values, std::string_view field)
    {
        for (std::string &value : values)
        {
            value = golarion::normalize(value);
        }
        std::ranges::sort(values);
        if (std::ranges::adjacent_find(values) != values.end())
        {
            throw std::invalid_argument("item selector " + std::string(field) + " must not contain duplicates");
        }
    }

    std::int64_t totalWeight(const golarion::ItemDefinition &definition, int quantity)
    {
        if (definition.weightGrams != 0 && quantity > std::numeric_limits<std::int64_t>::max() / definition.weightGrams)
        {
            throw std::invalid_argument("item weight is out of range");
        }
        return definition.weightGrams * quantity;
    }

    std::int64_t totalVolume(const golarion::ItemDefinition &definition, int quantity)
    {
        if (definition.volumeMilliliters != 0 && quantity > std::numeric_limits<std::int64_t>::max() / definition.volumeMilliliters)
        {
            throw std::invalid_argument("item volume is out of range");
        }
        return definition.volumeMilliliters * quantity;
    }

}

namespace golarion
{
    std::string_view displayName(ItemEffectActivation activation)
    {
        switch (activation)
        {
            case ItemEffectActivation::Possessed:
                return "Posseduto";
            case ItemEffectActivation::Equipped:
                return "Equipaggiato";
        }

        throw std::invalid_argument("unknown item effect activation");
    }

    ItemSelector::ItemSelector(ItemSelectorDefinition definition)
        : definitionIds_(std::move(definition.definitionIds)),
          tags_(std::move(definition.tags))
    {
        normalizeSelectorValues(definitionIds_, "definition IDs");
        normalizeSelectorValues(tags_, "tags");
    }

    ItemInstance::ItemInstance(ItemInstanceDefinition definition)
        : id_(normalize(definition.id)),
          itemDefinition_(ItemDefinitionManager::instance().get(definition.itemDefinitionId)),
          quantity_(definition.quantity)
    {
        if (quantity_ < 1)
        {
            throw std::invalid_argument("item quantity must be at least 1");
        }
    }

    std::int64_t ItemInstance::weightGrams() const
    {
        return totalWeight(itemDefinition_.get(), quantity_);
    }

    std::int64_t ItemInstance::volumeMilliliters() const
    {
        return totalVolume(itemDefinition_.get(), quantity_);
    }

    bool ItemInstance::matches(const ItemSelector &selector) const
    {
        if (selector.definitionIds_.empty() && selector.tags_.empty())
        {
            return true;
        }

        const ItemDefinition &definition = itemDefinition_.get();
        if (std::ranges::binary_search(selector.definitionIds_, definition.id))
        {
            return true;
        }
        return std::ranges::any_of(selector.tags_, [&definition](const std::string &tag)
        {
            return std::ranges::binary_search(definition.tags, tag);
        });
    }

    std::int64_t ItemInstance::matchingQuantity(const ItemSelector &selector) const
    {
        return matches(selector) ? quantity_ : 0;
    }

    std::vector<ItemEffectCleanup> ItemInstance::applyEffects(ResourceManager &resourceManager, ItemEffectActivation activation) const
    {
        std::vector<ItemEffectCleanup> cleanups;
        try
        {
            for (const ItemEffectDefinition &effect : itemDefinition_.get().effects)
            {
                if (effect.activation != activation)
                {
                    continue;
                }

                ItemEffectCleanup cleanup = effect.apply(resourceManager, ItemEffectContext{
                    .instanceId = itemEffectInstanceId(id_, effect.id),
                    .itemInstanceId = id_,
                    .itemDefinitionId = itemDefinition_.get().id,
                    .effectId = effect.id,
                    .source = itemDefinition_.get().name,
                    .quantity = quantity_
                });
                if (!cleanup)
                {
                    throw std::invalid_argument("item effect cleanup callback must not be empty: " + effect.id);
                }
                cleanups.push_back(std::move(cleanup));
            }
        }
        catch (...)
        {
            removeEffects(cleanups);
            throw;
        }
        return cleanups;
    }

    void ItemInstance::removeEffects(std::vector<ItemEffectCleanup> &cleanups) const noexcept
    {
        for (auto cleanup = cleanups.rbegin(); cleanup != cleanups.rend(); ++cleanup)
        {
            (*cleanup)();
        }
        cleanups.clear();
    }

}
