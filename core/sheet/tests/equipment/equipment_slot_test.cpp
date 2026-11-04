#include "golarion/equipment/equipment_slot.hpp"

#include <array>
#include <cassert>
#include <cstddef>

int main()
{
    using namespace golarion;

    constexpr std::array slots{
        EquipmentSlot::Ring,
        EquipmentSlot::Armor,
        EquipmentSlot::Belt,
        EquipmentSlot::Neck,
        EquipmentSlot::Body,
        EquipmentSlot::Headband,
        EquipmentSlot::Hands,
        EquipmentSlot::Eyes,
        EquipmentSlot::Feet,
        EquipmentSlot::Wrists,
        EquipmentSlot::Shield,
        EquipmentSlot::Shoulders,
        EquipmentSlot::Head,
        EquipmentSlot::Chest
    };

    std::size_t totalCapacity = 0;
    for (const EquipmentSlot slot : slots)
    {
        assert(!displayName(slot).empty());
        totalCapacity += capacity(slot);
    }

    assert(slots.size() == 14);
    assert(totalCapacity == 15);
    assert(capacity(EquipmentSlot::Ring) == 2);
    assert(capacity(EquipmentSlot::Armor) == 1);
    assert(displayName(EquipmentSlot::Headband) == "Fronte");

    return 0;
}
