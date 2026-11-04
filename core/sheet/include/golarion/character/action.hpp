#pragma once

#include "golarion/view/actions_view.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view ActionGrantsResource = "action.grants";
    inline constexpr std::string_view ActionCategoriesResource = "action.categories";
    inline constexpr std::string_view ActionInhibitionsResource = "action.inhibitions";
    inline constexpr std::string_view ActionNotesResource = "action.notes";
    inline constexpr std::string_view ActionCostReplacementsResource = "action.costReplacements";

    class ResourceManager;

    enum class ActionCost
    {
        Standard,
        Move,
        FullRound,
        Swift,
        Immediate,
        Free,
        ReplacesAttack,
        NotAnAction
    };

    std::string_view displayName(ActionCost cost);

    struct ActionCategoryDefinition
    {
        std::string id;
        std::string source;
        std::string name;
    };

    class ActionCategory final
    {
    public:
        explicit ActionCategory(ActionCategoryDefinition definition);

    private:
        friend class ActionManager;

        std::string id_;
        std::string source_;
        std::string name_;
    };

    struct ActionDefinition
    {
        std::string id;
        std::string source;
        std::string categoryId;
        std::string name;
        std::string description;
        ActionCost cost;
        std::vector<std::string> tags;
    };

    class Action final
    {
    public:
        explicit Action(ActionDefinition definition);

    private:
        friend class ActionManager;
        friend class ActionSelector;

        std::string id_;
        std::string source_;
        std::string categoryId_;
        std::string name_;
        std::string description_;
        ActionCost cost_;
        std::vector<std::string> tags_;
    };

    struct ActionSelectorDefinition
    {
        std::optional<std::string> actionId = std::nullopt;
        std::optional<std::string> categoryId = std::nullopt;
        std::optional<ActionCost> cost = std::nullopt;
        std::vector<std::string> requiredTags{};
    };

    class ActionSelector final
    {
    public:
        explicit ActionSelector(ActionSelectorDefinition definition = {});

    private:
        friend class ActionManager;

        bool matches(const Action &action, ActionCost effectiveCost) const;

        std::optional<std::string> actionId_;
        std::optional<std::string> categoryId_;
        std::optional<ActionCost> cost_;
        std::vector<std::string> requiredTags_;
    };

    struct ActionInhibitionDefinition
    {
        std::string id;
        std::string source;
        ActionSelector selector;
        std::string reason;
    };

    class ActionInhibition final
    {
    public:
        explicit ActionInhibition(ActionInhibitionDefinition definition);

    private:
        friend class ActionManager;

        std::string id_;
        std::string source_;
        ActionSelector selector_;
        std::string reason_;
    };

    struct ActionNoteDefinition
    {
        std::string id;
        std::string source;
        std::string actionId;
        std::string text;
    };

    class ActionNote final
    {
    public:
        explicit ActionNote(ActionNoteDefinition definition);

    private:
        friend class ActionManager;

        std::string id_;
        std::string source_;
        std::string actionId_;
        std::string text_;
    };

    struct ActionCostReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string actionId;
        ActionCost cost;
    };

    class ActionCostReplacement final
    {
    public:
        explicit ActionCostReplacement(ActionCostReplacementDefinition definition);

    private:
        friend class ActionManager;

        std::string id_;
        std::string source_;
        std::string actionId_;
        ActionCost cost_;
    };

    class ActionManager final
    {
    public:
        explicit ActionManager(ResourceManager &resourceManager);

        ActionsView toView() const;
        std::optional<ActionView> actionView(std::string_view actionId) const;

    private:
        void addCategory(ActionCategory category);
        void removeCategory(std::string_view categoryId);
        void addAction(Action action);
        void removeAction(std::string_view actionId);
        void addInhibition(ActionInhibition inhibition);
        void removeInhibition(std::string_view inhibitionId);
        void addNote(ActionNote note);
        void removeNote(std::string_view noteId);
        void addCostReplacement(ActionCostReplacement replacement);
        void removeCostReplacement(std::string_view replacementId);
        ActionCost effectiveCost(const Action &action) const;
        void registerCanonicalActions();

        std::map<std::string, ActionCategory> categories_;
        std::map<std::string, Action> actions_;
        std::map<std::string, ActionInhibition> inhibitions_;
        std::map<std::string, ActionNote> notes_;
        std::map<std::string, ActionCostReplacement> costReplacements_;
    };
}
