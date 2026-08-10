#pragma once

#include "golarion/character/saving_throw.hpp"

namespace golarion
{
    struct SavingThrowSaveData
    {
        SavingThrowType type;
        int baseValue;
        AbilityType abilityType;
    };
}
