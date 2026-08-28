#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    class SpecialDefenses;

    struct SpellResistanceDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
        std::optional<std::string> applicability = std::nullopt;
        bool canBeLowered = true;
        std::vector<std::string> tags{};
    };

    class SpellResistance final
    {
    public:
        explicit SpellResistance(SpellResistanceDefinition definition);

    private:
        friend class SpellResistanceSelector;
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        std::string expression_;
        std::optional<std::string> applicability_;
        bool canBeLowered_;
        std::vector<std::string> tags_;
    };

    struct SpellResistanceSelectorDefinition
    {
        std::optional<std::string> grantId = std::nullopt;
        std::vector<std::string> anyTags{};
        std::vector<std::string> excludedGrantIds{};
    };

    class SpellResistanceSelector final
    {
    public:
        explicit SpellResistanceSelector(SpellResistanceSelectorDefinition definition = {});

    private:
        friend class SpecialDefenses;

        bool matches(const SpellResistance &spellResistance) const;

        std::optional<std::string> grantId_;
        std::vector<std::string> anyTags_;
        std::vector<std::string> excludedGrantIds_;
    };

    struct SpellResistanceAdjustmentDefinition
    {
        std::string id;
        std::string source;
        SpellResistanceSelector selector;
        std::string expression;
        std::string stackingGroup;
        std::optional<std::string> applicability = std::nullopt;
    };

    class SpellResistanceAdjustment final
    {
    public:
        explicit SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        SpellResistanceSelector selector_;
        std::string expression_;
        std::string stackingGroup_;
        std::optional<std::string> applicability_;
    };
}
