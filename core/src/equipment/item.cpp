#include "golarion/equipment/item.hpp"

#include "golarion/character/encumbrance.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <limits>
#include <stdexcept>

namespace
{
    std::int64_t totalWeight(const golarion::ItemDefinition &definition, int quantity)
    {
        if (definition.weightGrams != 0 && quantity > std::numeric_limits<std::int64_t>::max() / definition.weightGrams)
        {
            throw std::invalid_argument("item weight is out of range");
        }
        return definition.weightGrams * quantity;
    }

    std::string weightId(std::string_view itemInstanceId)
    {
        return "item." + std::string(itemInstanceId);
    }
}

namespace golarion
{
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

    void ItemInstance::registerWeight(ResourceManager &resourceManager) const
    {
        const ItemDefinition &definition = itemDefinition_.get();
        resourceManager.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
            .id = weightId(id_),
            .source = definition.name,
            .grams = totalWeight(definition, quantity_)
        }));
    }

    void ItemInstance::refreshWeight(ResourceManager &resourceManager) const
    {
        unregisterWeight(resourceManager);
        registerWeight(resourceManager);
    }

    void ItemInstance::unregisterWeight(ResourceManager &resourceManager) const
    {
        resourceManager.removeFromCollection(CarriedWeightsResource, weightId(id_));
    }
}
