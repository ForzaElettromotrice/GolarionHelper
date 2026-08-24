#include "golarion/equipment/equipment_slot.hpp"

#include <stdexcept>

namespace golarion
{
    std::string_view displayName(MagicItemSlot slot)
    {
        switch (slot)
        {
            case MagicItemSlot::Ring:
                return "Anello";
            case MagicItemSlot::Armor:
                return "Armatura";
            case MagicItemSlot::Belt:
                return "Cintura";
            case MagicItemSlot::Neck:
                return "Collo";
            case MagicItemSlot::Body:
                return "Corpo";
            case MagicItemSlot::Headband:
                return "Fronte";
            case MagicItemSlot::Hands:
                return "Mani";
            case MagicItemSlot::Eyes:
                return "Occhi";
            case MagicItemSlot::Feet:
                return "Piedi";
            case MagicItemSlot::Wrists:
                return "Polsi";
            case MagicItemSlot::Shield:
                return "Scudo";
            case MagicItemSlot::Shoulders:
                return "Spalle";
            case MagicItemSlot::Head:
                return "Testa";
            case MagicItemSlot::Chest:
                return "Torace";
        }
        throw std::invalid_argument("unknown magic item slot");
    }

    std::string_view displayName(HandSlot slot)
    {
        switch (slot)
        {
            case HandSlot::Main:
                return "Mano principale";
            case HandSlot::Off:
                return "Mano secondaria";
        }
        throw std::invalid_argument("unknown hand slot");
    }

    std::string_view displayName(HandUsage usage)
    {
        switch (usage)
        {
            case HandUsage::None:
                return "Nessuna";
            case HandUsage::OneHand:
                return "Una mano";
            case HandUsage::TwoHands:
                return "Due mani";
        }
        throw std::invalid_argument("unknown hand usage");
    }

    std::size_t capacity(MagicItemSlot slot)
    {
        switch (slot)
        {
            case MagicItemSlot::Ring:
                return 2;
            case MagicItemSlot::Armor:
            case MagicItemSlot::Belt:
            case MagicItemSlot::Neck:
            case MagicItemSlot::Body:
            case MagicItemSlot::Headband:
            case MagicItemSlot::Hands:
            case MagicItemSlot::Eyes:
            case MagicItemSlot::Feet:
            case MagicItemSlot::Wrists:
            case MagicItemSlot::Shield:
            case MagicItemSlot::Shoulders:
            case MagicItemSlot::Head:
            case MagicItemSlot::Chest:
                return 1;
        }
        throw std::invalid_argument("unknown magic item slot");
    }
}
