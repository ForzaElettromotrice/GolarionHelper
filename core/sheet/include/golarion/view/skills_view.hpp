#pragma once

#include "golarion/view/skill_view.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct ArmorCheckPenaltySourceView
    {
        std::string id;
        std::string source;
        std::string expression;
        int resolvedValue;
        bool constraining;
    };

    struct ArmorCheckPenaltyView
    {
        int total;
        std::vector<ArmorCheckPenaltySourceView> sources;
    };

    struct SkillsView
    {
        ArmorCheckPenaltyView armorCheckPenalty;
        std::vector<SkillView> skills;
        int maxRanks = 0;
        int spentRanks = 0;
        int remainingRanks = 0;
    };
}
