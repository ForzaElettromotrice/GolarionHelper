#pragma once

#include "golarion/view/saving_throw_view.hpp"

#include <vector>

namespace golarion
{
    struct SavingThrowsView
    {
        std::vector<SavingThrowView> savingThrows;
    };
}
