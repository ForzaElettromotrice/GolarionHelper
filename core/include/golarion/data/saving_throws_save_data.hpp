#pragma once

#include "golarion/data/saving_throw_save_data.hpp"

#include <vector>

namespace golarion
{
    struct SavingThrowsSaveData
    {
        std::vector<SavingThrowSaveData> savingThrows;
    };
}
