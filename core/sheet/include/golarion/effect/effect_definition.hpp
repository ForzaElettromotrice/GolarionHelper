#pragma once

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/size.hpp"
#include "golarion/resource/modifier.hpp"

#include <optional>
#include <string>
#include <variant>

namespace golarion
{
    struct ModifierEffectDefinition
    {
        std::string id;
        std::string resource;
        std::string description;
        ModifierType type;
        std::optional<BonusType> bonusType;
        std::string expression;
        std::optional<std::string> condition = std::nullopt;
    };

    struct ContributionEffectDefinition
    {
        std::string id;
        std::string resource;
        std::string expression;
    };

    struct SizeBaseEffectDefinition
    {
        std::string id;
        SizeCategory category;
    };

    struct MovementGrantEffectDefinition
    {
        std::string id;
        MovementType type;
        std::string baseSpeedExpression;
        std::optional<Maneuverability> maneuverability = std::nullopt;
        bool affectedByArmor;
        bool affectedByLoad;
        bool supportsRunning = false;
    };

    struct CarryingBodyTypeEffectDefinition
    {
        std::string id;
        CarryingBodyType type;
    };

    struct ReminderEffectDefinition
    {
        std::string id;
        std::string message;
    };

    using EffectDefinition = std::variant<
        ModifierEffectDefinition,
        ContributionEffectDefinition,
        SizeBaseEffectDefinition,
        MovementGrantEffectDefinition,
        CarryingBodyTypeEffectDefinition,
        ReminderEffectDefinition>;

    inline const std::string &effectId(const EffectDefinition &definition)
    {
        return std::visit([](const auto &effect) -> const std::string &
        {
            return effect.id;
        }, definition);
    }
}
