#pragma once

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/view/modifier_set_view.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct CarryingBodyTypeBaseView
    {
        std::string id;
        std::string source;
        CarryingBodyType type;
    };

    struct CarryingCapacityMultiplierView
    {
        std::string id;
        std::string source;
        std::string stackingGroup;
        int numerator;
        int denominator;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct CarryingCapacityView
    {
        int strength;
        ModifierSetView strengthModifiers;
        int effectiveStrength;
        CarryingBodyType bodyType;
        std::optional<CarryingBodyTypeBaseView> bodyTypeBase;
        SizeCategory size;
        std::int64_t baseLightLoadMaxGrams;
        std::int64_t baseMediumLoadMaxGrams;
        std::int64_t baseHeavyLoadMaxGrams;
        std::int64_t combinedMultiplierNumerator;
        std::int64_t combinedMultiplierDenominator;
        std::vector<CarryingCapacityMultiplierView> multipliers;
        std::int64_t lightLoadMaxGrams;
        std::int64_t mediumLoadMaxGrams;
        std::int64_t heavyLoadMaxGrams;
        std::int64_t liftFromGroundMaxGrams;
        std::int64_t pushOrDragMaxGrams;
    };
}
