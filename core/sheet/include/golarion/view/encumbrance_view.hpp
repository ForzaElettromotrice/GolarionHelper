#pragma once

#include "golarion/character/encumbrance.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct CarriedWeightView
    {
        std::string id;
        std::string source;
        std::int64_t grams;
    };

    struct LoadEffectsView
    {
        std::optional<int> maximumDexterityBonus;
        int armorCheckPenalty;
        bool losesDexterityBonusToArmorClass;
        bool reducesMovement;
        std::optional<int> movementSpeedLimitUnits;
        int runMultiplierPenalty;
        bool preventsRunning;
    };

    struct EncumbranceView
    {
        std::int64_t totalWeightGrams;
        std::vector<CarriedWeightView> weights;
        std::int64_t lightLoadMaxGrams;
        std::int64_t mediumLoadMaxGrams;
        std::int64_t heavyLoadMaxGrams;
        LoadCategory category;
        LoadEffectsView effects;
        bool withinLiftFromGroundLimit;
        bool withinPushOrDragLimit;
    };
}
