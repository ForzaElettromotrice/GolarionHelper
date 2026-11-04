#include "golarion/resource/modifier.hpp"

#include "golarion/view/modifier_view.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    std::string generateUuid()
    {
        static thread_local std::mt19937_64 generator(std::random_device{}());
        static thread_local std::uniform_int_distribution<unsigned int> distribution(0, 255);

        std::array<std::uint8_t, 16> bytes{};
        for (std::uint8_t &byte : bytes)
        {
            byte = static_cast<std::uint8_t>(distribution(generator));
        }

        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);

        std::ostringstream stream;
        stream << std::hex << std::setfill('0');
        for (std::size_t index = 0; index < bytes.size(); ++index)
        {
            if (index == 4 || index == 6 || index == 8 || index == 10)
            {
                stream << '-';
            }
            stream << std::setw(2) << static_cast<unsigned int>(bytes[index]);
        }

        return stream.str();
    }

}

namespace golarion
{
    std::string_view displayName(ModifierType type)
    {
        switch (type)
        {
            case ModifierType::Bonus:
                return "Bonus";
            case ModifierType::Penalty:
                return "Penalità";
        }

        throw std::invalid_argument("unknown modifier type");
    }

    std::string_view displayName(StackingRule rule)
    {
        switch (rule)
        {
            case StackingRule::HighestOnly:
                return "Solo il più alto";
            case StackingRule::Stacks:
                return "Cumulabile";
            case StackingRule::StacksUnlessSameSource:
                return "Cumulabile tra sorgenti diverse";
        }

        throw std::invalid_argument("unknown stacking rule");
    }

    std::string_view displayName(BonusType type)
    {
        switch (type)
        {
            case BonusType::Generic:
                return "Generico";
            case BonusType::Alchemical:
                return "Alchemico";
            case BonusType::Armor:
                return "Armatura";
            case BonusType::NaturalArmor:
                return "Armatura naturale";
            case BonusType::Circumstance:
                return "Circostanza";
            case BonusType::Insight:
                return "Cognitivo";
            case BonusType::Competence:
                return "Competenza";
            case BonusType::Deflection:
                return "Deviazione";
            case BonusType::Luck:
                return "Fortuna";
            case BonusType::Inherent:
                return "Intrinseco";
            case BonusType::Morale:
                return "Morale";
            case BonusType::Enhancement:
                return "Potenziamento";
            case BonusType::Profane:
                return "Profano";
            case BonusType::Racial:
                return "Razziale";
            case BonusType::Resistance:
                return "Resistenza";
            case BonusType::Sacred:
                return "Sacro";
            case BonusType::Dodge:
                return "Schivare";
            case BonusType::Shield:
                return "Scudo";
            case BonusType::Size:
                return "Taglia";
        }

        throw std::invalid_argument("unknown bonus type");
    }

    StackingRule stackingRule(BonusType type)
    {
        switch (type)
        {
            case BonusType::Generic:
            case BonusType::Racial:
            case BonusType::Dodge:
                return StackingRule::Stacks;
            case BonusType::Circumstance:
                return StackingRule::StacksUnlessSameSource;
            default:
                return StackingRule::HighestOnly;
        }
    }

    Modifier::Modifier(ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression)
        : Modifier(type, std::move(source), std::move(description), bonusType, std::move(expression), std::nullopt, {})
    {
    }

    Modifier::Modifier(ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition)
        : Modifier(type, std::move(source), std::move(description), bonusType, std::move(expression), std::move(condition), {})
    {
    }

    Modifier::Modifier(ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition, std::vector<Requirement> requirements)
        : Modifier(generateUuid(), type, std::move(source), std::move(description), bonusType, std::move(expression), std::move(condition), std::move(requirements))
    {
    }

    Modifier::Modifier(std::string id, ModifierType type, std::string source, std::string description, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition, std::vector<Requirement> requirements)
        : id_(normalize(id)),
          type_(type),
          bonusType_(bonusType),
          requirements_(std::move(requirements))
    {
        if (type == ModifierType::Bonus && !bonusType.has_value())
        {
            throw std::invalid_argument("bonus type is required for bonus modifiers");
        }
        if (type == ModifierType::Penalty && bonusType.has_value())
        {
            throw std::invalid_argument("bonus type must be absent for penalty modifiers");
        }

        source_ = normalize(source);
        description_ = normalize(description);
        expression_ = normalize(expression);
        if (condition.has_value())
        {
            condition_ = normalize(*condition);
        }
    }

    ModifierView Modifier::toView(ResourceManager &resourceManager) const
    {
        std::vector<RequirementView> requirementViews;
        requirementViews.reserve(requirements_.size());
        bool active = true;
        for (const Requirement &requirement : requirements_)
        {
            RequirementView view = requirement.toView(resourceManager);
            active = active && view.satisfied;
            requirementViews.push_back(std::move(view));
        }

        return ModifierView{
            .id = id_,
            .type = type_,
            .source = source_,
            .description = description_,
            .bonusType = bonusType_,
            .expression = expression_,
            .condition = condition_,
            .resolvedValue = resolveValue(resourceManager),
            .active = active,
            .requirements = std::move(requirementViews)
        };
    }

    int Modifier::resolveValue(ResourceManager &resourceManager) const
    {
        const int value = resourceManager.evaluateExpression(expression_);
        if (value < 0)
        {
            throw std::invalid_argument("modifier expression must not resolve to a negative value: " + expression_);
        }
        return value;
    }

    bool Modifier::requirementsSatisfied(ResourceManager &resourceManager) const
    {
        for (const Requirement &requirement : requirements_)
        {
            if (!requirement.isSatisfied(resourceManager))
            {
                return false;
            }
        }
        return true;
    }

    const std::string &Modifier::id() const
    {
        return id_;
    }

    ModifierType Modifier::type() const
    {
        return type_;
    }

    const std::string &Modifier::source() const
    {
        return source_;
    }

    const std::string &Modifier::description() const
    {
        return description_;
    }

    std::optional<BonusType> Modifier::bonusType() const
    {
        return bonusType_;
    }

    const std::string &Modifier::expression() const
    {
        return expression_;
    }

    const std::optional<std::string> &Modifier::condition() const
    {
        return condition_;
    }

    const std::vector<Requirement> &Modifier::requirements() const
    {
        return requirements_;
    }
}
