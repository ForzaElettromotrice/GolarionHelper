#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    enum class ActionCost;

    struct ActionInhibitionView
    {
        std::string id;
        std::string source;
        std::string reason;
    };

    struct ActionNoteView
    {
        std::string id;
        std::string source;
        std::string text;
    };

    struct ActionCostReplacementView
    {
        std::string id;
        std::string source;
        ActionCost cost;
    };

    struct ActionView
    {
        std::string id;
        std::string source;
        std::string categoryId;
        std::string name;
        std::string description;
        ActionCost baseCost;
        ActionCost effectiveCost;
        std::vector<std::string> tags;
        std::vector<ActionNoteView> notes;
        std::vector<ActionCostReplacementView> costReplacements;
        bool usable;
        std::vector<ActionInhibitionView> inhibitions;
    };

    struct ActionCategoryView
    {
        std::string id;
        std::string source;
        std::string name;
        std::vector<ActionView> actions;
    };

    struct ActionsView
    {
        std::vector<ActionCategoryView> categories;
    };
}
