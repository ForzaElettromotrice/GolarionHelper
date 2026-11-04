#pragma once

#include "golarion/character/size.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct SizeBaseView
    {
        std::string id;
        std::string source;
        SizeCategory category;
    };

    struct SizeReplacementView
    {
        std::string id;
        std::string source;
        SizeCategory category;
        bool acceptsAdjustments;
        bool applied;
    };

    struct SizeAdjustmentView
    {
        std::string id;
        std::string source;
        int steps;
        bool applied;
        std::optional<std::string> notAppliedReason;
    };

    struct SizeView
    {
        SizeCategory baseCategory;
        std::optional<SizeCategory> replacementCategory;
        bool acceptsAdjustments;
        int selectedAdjustmentSteps;
        int appliedAdjustmentSteps;
        SizeCategory effectiveCategory;
        int armorClassModifier;
        int attackModifier;
        int combatManeuverModifier;
        int stealthModifier;
        int flyModifier;
        std::optional<SizeBaseView> base;
        std::vector<SizeReplacementView> replacements;
        std::vector<SizeAdjustmentView> adjustments;
    };
}
