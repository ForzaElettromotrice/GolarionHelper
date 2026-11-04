#pragma once

#include "golarion/equipment/coin.hpp"
#include "golarion/view/money_view.hpp"

#include <string_view>

namespace golarion
{
    class Inventory;

    class Money final
    {
    public:
        explicit Money(Inventory &inventory);

        void add(CoinDenomination denomination, int quantity, std::string_view containerId);
        void remove(CoinDenomination denomination, int quantity, std::string_view containerId);
        MoneyView toView() const;

    private:
        Inventory &inventory_;
    };
}
