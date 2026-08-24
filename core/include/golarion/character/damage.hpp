#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    struct DamageDiceView;

    enum class DamageType
    {
        Bludgeoning,
        Piercing,
        Slashing,
        Acid,
        Cold,
        Electricity,
        Fire,
        Sonic,
        Force,
        PositiveEnergy,
        NegativeEnergy,
        Untyped
    };

    enum class DamageTypeMode
    {
        All,
        Choice
    };

    enum class DamageCriticalRule
    {
        Multiplied,
        NotMultiplied,
        CriticalOnly
    };

    enum class DamageTrait
    {
        Precision,
        Bleed,
        NonLethal
    };

    enum class DamageComponentRole
    {
        Base,
        Additional
    };

    enum class DamageDiceAdjustmentType
    {
        ProgressionSteps,
        DiceCountMultiplier,
        Set
    };

    enum class DamageComponentOriginFilter
    {
        Any,
        Intrinsic,
        ExternalGrant
    };

    std::string_view displayName(DamageType type);
    std::string_view displayName(DamageTypeMode mode);
    std::string_view displayName(DamageCriticalRule rule);
    std::string_view displayName(DamageTrait trait);
    std::string_view displayName(DamageComponentRole role);
    std::string_view displayName(DamageDiceAdjustmentType type);
    std::string_view displayName(DamageComponentOriginFilter filter);

    struct DamageDiceDefinition
    {
        int diceCount;
        int dieSize;
    };

    class DamageDice final
    {
    public:
        explicit DamageDice(DamageDiceDefinition definition);

        std::string toString() const;
        DamageDiceView toView() const;
        DamageDice adjustedByProgression(int steps) const;
        DamageDice multiplied(int multiplier) const;

    private:
        int diceCount_;
        int dieSize_;
    };

    struct DamageComponentDefinition
    {
        std::string id;
        std::string source;
        DamageComponentRole role;
        DamageDice dice;
        std::vector<DamageType> types;
        DamageTypeMode typeMode;
        DamageCriticalRule criticalRule;
        std::vector<DamageTrait> traits;
    };

    class DamageComponent final
    {
    public:
        explicit DamageComponent(DamageComponentDefinition definition);

    private:
        friend class StrikeGrant;
        friend class Strikes;

        std::string id_;
        std::string source_;
        DamageComponentRole role_;
        DamageDice dice_;
        std::vector<DamageType> types_;
        DamageTypeMode typeMode_;
        DamageCriticalRule criticalRule_;
        std::vector<DamageTrait> traits_;
    };
}
