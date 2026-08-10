#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    class ModifierSet;
    class ResourceManager;
    struct ModifierSaveData;
    struct ModifierView;

    enum class ModifierType
    {
        Bonus,
        Penalty
    };

    enum class StackingRule
    {
        HighestOnly,
        Stacks,
        StacksUnlessSameSource
    };

    enum class BonusType
    {
        Alchemical,
        Armor,
        NaturalArmor,
        Circumstance,
        Insight,
        Competence,
        Deflection,
        Luck,
        Inherent,
        Morale,
        Enhancement,
        Profane,
        Racial,
        Resistance,
        Sacred,
        Dodge,
        Shield,
        Size
    };

    std::string_view displayName(ModifierType type);
    std::string_view displayName(StackingRule rule);
    std::string_view displayName(BonusType type);
    StackingRule stackingRule(BonusType type);

    class Modifier final
    {
    public:
        Modifier(ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression);
        Modifier(ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition);
        explicit Modifier(const ModifierSaveData &data);

        ModifierView toView(ResourceManager &resourceManager) const;
        ModifierSaveData toSaveData() const;

        const std::string &id() const;
        ModifierType type() const;
        const std::string &source() const;
        const std::string &description() const;
        std::optional<BonusType> bonusType() const;
        const std::string &expression() const;
        const std::optional<std::string> &condition() const;

    private:
        friend class ModifierSet;

        Modifier(std::string id, ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition);
        ModifierView toView(int resolvedValue) const;
        int resolveValue(ResourceManager &resourceManager) const;

        std::string id_;
        ModifierType type_;
        std::string source_;
        std::string description_;
        std::optional<BonusType> bonusType_;
        std::string expression_;
        std::optional<std::string> condition_;
    };
}
