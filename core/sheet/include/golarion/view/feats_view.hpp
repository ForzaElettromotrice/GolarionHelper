#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct FeatChoiceOptionView
    {
        std::string id;
        std::string name;
    };

    struct FeatChoiceView
    {
        std::string id;
        std::string prompt;
        std::vector<FeatChoiceOptionView> selectedOptions;
    };

    struct FeatView
    {
        std::string slotId;
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> types;
        std::vector<FeatChoiceView> choices;
        bool active;
    };

    struct FeatSlotView
    {
        std::string id;
        std::string source;
        std::optional<std::string> featId;
        // No type and no IDs means unrestricted.
        std::optional<std::string> allowedType;
        std::vector<std::string> allowedFeatIds;
    };

    struct FeatsView
    {
        std::vector<FeatSlotView> slots;
        std::vector<FeatView> feats;
    };
}
