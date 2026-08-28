#pragma once

#include <string_view>

namespace golarion
{
    enum class CoinDenomination
    {
        Copper,
        Silver,
        Gold,
        Platinum
    };

    std::string_view displayName(CoinDenomination denomination);
    std::string_view coinItemDefinitionId(CoinDenomination denomination);
}
