#pragma once

#include <string>
#include <vector>

namespace golarion
{
    struct AttackAssignmentData
    {
        std::string slotId;
        std::string strikeGrantId;
    };

    struct AttackData
    {
        std::string id;
        std::string name;
        std::string routineId;
        std::vector<AttackAssignmentData> assignments;
    };
}
