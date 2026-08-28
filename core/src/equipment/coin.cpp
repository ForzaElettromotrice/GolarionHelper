#include "golarion/equipment/coin.hpp"

#include <stdexcept>

namespace golarion
{
    std::string_view displayName(CoinDenomination denomination)
    {
        switch (denomination)
        {
            case CoinDenomination::Copper:
                return "Moneta di rame";
            case CoinDenomination::Silver:
                return "Moneta d'argento";
            case CoinDenomination::Gold:
                return "Moneta d'oro";
            case CoinDenomination::Platinum:
                return "Moneta di platino";
        }
        throw std::invalid_argument("unknown coin denomination");
    }

    std::string_view coinItemDefinitionId(CoinDenomination denomination)
    {
        switch (denomination)
        {
            case CoinDenomination::Copper:
                return "copperCoin";
            case CoinDenomination::Silver:
                return "silverCoin";
            case CoinDenomination::Gold:
                return "goldCoin";
            case CoinDenomination::Platinum:
                return "platinumCoin";
        }
        throw std::invalid_argument("unknown coin denomination");
    }
}
