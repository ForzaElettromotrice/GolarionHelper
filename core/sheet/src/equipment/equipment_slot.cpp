#include "golarion/equipment/equipment_slot.hpp"

#include <stdexcept>

namespace golarion
{
    std::string_view displayName(EquipmentSlot slot)
    {
        switch (slot)
        {
            case EquipmentSlot::Ring:
                return "Anello";
            case EquipmentSlot::Armor:
                return "Armatura";
            case EquipmentSlot::Belt:
                return "Cintura";
            case EquipmentSlot::Neck:
                return "Collo";
            case EquipmentSlot::Body:
                return "Corpo";
            case EquipmentSlot::Headband:
                return "Fronte";
            case EquipmentSlot::Hands:
                return "Mani";
            case EquipmentSlot::Eyes:
                return "Occhi";
            case EquipmentSlot::Feet:
                return "Piedi";
            case EquipmentSlot::Wrists:
                return "Polsi";
            case EquipmentSlot::Shield:
                return "Scudo";
            case EquipmentSlot::Shoulders:
                return "Spalle";
            case EquipmentSlot::Head:
                return "Testa";
            case EquipmentSlot::Chest:
                return "Torace";
        }

        throw std::invalid_argument("unknown equipment slot");
    }

    std::size_t capacity(EquipmentSlot slot)
    {
        switch (slot)
        {
            case EquipmentSlot::Ring:
                return 2;
            case EquipmentSlot::Armor:
            case EquipmentSlot::Belt:
            case EquipmentSlot::Neck:
            case EquipmentSlot::Body:
            case EquipmentSlot::Headband:
            case EquipmentSlot::Hands:
            case EquipmentSlot::Eyes:
            case EquipmentSlot::Feet:
            case EquipmentSlot::Wrists:
            case EquipmentSlot::Shield:
            case EquipmentSlot::Shoulders:
            case EquipmentSlot::Head:
            case EquipmentSlot::Chest:
                return 1;
        }

        throw std::invalid_argument("unknown equipment slot");
    }
}
