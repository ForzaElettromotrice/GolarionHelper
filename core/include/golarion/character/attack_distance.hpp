#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view AttackDistanceAdjustmentsResource = "attack.distanceAdjustments";

    enum class AttackMode;
    enum class AttackTag;

    enum class AttackDistanceProperty
    {
        MinimumReach,
        MaximumReach,
        RangeIncrement,
        MaximumRangeIncrements,
        RangePenaltyPerAdditionalIncrement
    };

    enum class AttackDistanceAdjustmentType
    {
        Multiplier,
        Minimum,
        Maximum
    };

    std::string_view displayName(AttackDistanceProperty property);
    std::string_view displayName(AttackDistanceAdjustmentType type);
    std::string attackDistanceResourceName(AttackDistanceProperty property);
    std::string attackDistanceResourceName(AttackDistanceProperty property, AttackMode mode);
    std::string attackDistanceResourceName(AttackDistanceProperty property, AttackTag tag);
    std::string attackDistanceResourceName(AttackDistanceProperty property, std::string_view grantId);

    struct AttackReachDefinition
    {
        int minimumUnits;
        int maximumUnits;
    };

    struct AttackRangeDefinition
    {
        int incrementUnits;
        int maximumIncrements;
        int penaltyPerAdditionalIncrement;
    };

    struct AttackDistanceAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AttackDistanceAdjustmentType type;
        std::string expression;
        std::optional<std::string> condition;
    };

    class AttackDistanceAdjustment final
    {
    public:
        explicit AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition definition);

    private:
        friend class Attacks;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        AttackDistanceAdjustmentType type_;
        std::string expression_;
        std::optional<std::string> condition_;
    };
}
