#include "golarion/equipment/equipment_slot.hpp"

#include <array>
#include <cassert>
#include <cstddef>

int main()
{
    using namespace golarion;

    constexpr std::array slots{
        MagicItemSlot::Ring,
        MagicItemSlot::Armor,
        MagicItemSlot::Belt,
        MagicItemSlot::Neck,
        MagicItemSlot::Body,
        MagicItemSlot::Headband,
        MagicItemSlot::Hands,
        MagicItemSlot::Eyes,
        MagicItemSlot::Feet,
        MagicItemSlot::Wrists,
        MagicItemSlot::Shield,
        MagicItemSlot::Shoulders,
        MagicItemSlot::Head,
        MagicItemSlot::Chest
    };

    std::size_t totalCapacity = 0;
    for (MagicItemSlot slot : slots)
    {
        assert(!displayName(slot).empty());
        totalCapacity += capacity(slot);
    }

    assert(slots.size() == 14);
    assert(totalCapacity == 15);
    assert(capacity(MagicItemSlot::Ring) == 2);
    assert(capacity(MagicItemSlot::Armor) == 1);
    assert(displayName(MagicItemSlot::Headband) == "Fronte");
    assert(displayName(HandSlot::Main) == "Mano principale");
    assert(displayName(HandSlot::Off) == "Mano secondaria");
    assert(displayName(HandUsage::None) == "Nessuna");
    assert(displayName(HandUsage::OneHand) == "Una mano");
    assert(displayName(HandUsage::TwoHands) == "Due mani");

    return 0;
}
