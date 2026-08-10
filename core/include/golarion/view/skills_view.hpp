#pragma once

#include "golarion/view/skill_view.hpp"

#include <vector>

namespace golarion
{
    struct SkillsView
    {
        std::vector<SkillView> skills;
    };
}
