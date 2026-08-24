#include "golarion/equipment/item_location.hpp"

#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace golarion
{
    ContainerItemLocation::ContainerItemLocation(std::string_view containerItemId) : containerItemId_(normalize(containerItemId))
    {
    }

    EquippedItemLocation::EquippedItemLocation(EquippedItemLocationDefinition definition)
        : magicItemSlot_(definition.magicItemSlot), occupiedHands_(std::move(definition.occupiedHands))
    {
        if (!magicItemSlot_.has_value() && occupiedHands_.empty())
        {
            throw std::invalid_argument("equipped item location must occupy a magic item slot or at least one hand");
        }

        std::ranges::sort(occupiedHands_);
        if (std::ranges::adjacent_find(occupiedHands_) != occupiedHands_.end())
        {
            throw std::invalid_argument("equipped item location cannot occupy the same hand more than once");
        }
    }
}
