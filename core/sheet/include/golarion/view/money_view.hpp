#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace golarion
{
    struct CoinAmountsView
    {
        std::int64_t copper;
        std::int64_t silver;
        std::int64_t gold;
        std::int64_t platinum;
        std::int64_t weightGrams;
    };

    struct ContainerMoneyView
    {
        std::string containerId;
        std::string containerName;
        CoinAmountsView amounts;
    };

    struct MoneyView
    {
        std::vector<ContainerMoneyView> containers;
        CoinAmountsView total;
    };
}
