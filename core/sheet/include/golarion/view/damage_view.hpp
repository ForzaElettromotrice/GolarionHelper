#pragma once

#include "golarion/character/damage.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct DamageDiceView
    {
        int diceCount;
        int dieSize;
        std::string expression;
    };

    struct DamageDiceAdjustmentView
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        DamageComponentRole targetRole;
        DamageComponentOriginFilter targetOrigin;
        std::optional<std::string> targetComponentGrantId;
        std::optional<std::string> targetComponentId;
        DamageDiceAdjustmentType type;
        std::optional<std::string> expression;
        std::optional<int> resolvedValue;
        std::optional<DamageDiceView> setDice;
        std::optional<std::string> condition;
    };

    struct ConditionalDamageDiceView
    {
        std::string condition;
        DamageDiceView dice;
    };

    struct DamageComponentView
    {
        std::string id;
        std::string source;
        std::optional<std::string> grantId;
        std::optional<std::string> grantSource;
        DamageComponentRole role;
        DamageDiceView baseDice;
        DamageDiceView effectiveDice;
        std::vector<ConditionalDamageDiceView> conditionalDice;
        std::vector<DamageDiceAdjustmentView> diceAdjustments;
        std::vector<DamageType> types;
        DamageTypeMode typeMode;
        DamageCriticalRule criticalRule;
        std::vector<DamageTrait> traits;
        bool includedInNormalDamage;
        int criticalOccurrences;
    };
}
