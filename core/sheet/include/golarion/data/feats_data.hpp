#pragma once

#include <map>
#include <string>
#include <vector>

namespace golarion
{
    struct FeatSelectionData
    {
        std::string slotId;
        std::string featId;
        std::map<std::string, std::vector<std::string>> choices;
    };

    struct FeatsData
    {
        std::vector<FeatSelectionData> selections;
        std::vector<std::string> extraSlotIds;
    };
}
