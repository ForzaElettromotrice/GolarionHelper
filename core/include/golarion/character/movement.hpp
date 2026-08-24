#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view MovementGrantsResource = "movement.grants";
    inline constexpr std::string_view MovementAdjustmentsResource = "movement.adjustments";
    inline constexpr std::string_view RunAdjustmentsResource = "movement.runAdjustments";
    inline constexpr std::string_view HeavyArmorOrLoadRunPenaltyGroup = "heavyArmorOrLoad";

    class ResourceManager;
    struct MovementView;

    enum class MovementType
    {
        Land,
        Climb,
        Swim,
        Burrow,
        Fly
    };

    enum class Maneuverability
    {
        Clumsy,
        Poor,
        Average,
        Good,
        Perfect
    };

    enum class MovementAdjustmentType
    {
        // The expression resolves to a percentage: 50 halves speed, 200 doubles it.
        SpeedMultiplier,
        ReducedByArmorOrLoad,
        SpeedLimit,
        ManeuverabilityChange,
        Block
    };

    enum class RunAdjustmentType
    {
        Increase,
        Penalty,
        Replacement,
        Block
    };

    std::string_view displayName(MovementType type);
    std::string_view displayName(Maneuverability maneuverability);
    std::string_view displayName(MovementAdjustmentType type);
    std::string_view displayName(RunAdjustmentType type);
    int flyCheckModifier(Maneuverability maneuverability);
    std::string movementResourceName(MovementType type);
    std::string movementResourceName(MovementType type, std::string_view grantId);

    struct MovementGrantDefinition
    {
        std::string id;
        std::string source;
        MovementType type;
        std::string baseSpeedExpression;
        std::optional<Maneuverability> maneuverability;
        bool affectedByArmor;
        bool affectedByLoad;
        bool supportsRunning = false;
    };

    class MovementGrant final
    {
    public:
        explicit MovementGrant(MovementGrantDefinition definition);

    private:
        friend class Movement;

        std::string id_;
        std::string source_;
        MovementType type_;
        std::string baseSpeedExpression_;
        std::optional<Maneuverability> maneuverability_;
        bool affectedByArmor_;
        bool affectedByLoad_;
        bool supportsRunning_;
    };

    struct MovementSelector
    {
        std::optional<MovementType> type;
        std::optional<std::string> grantId;
        bool affectedByArmorOnly = false;
        bool affectedByLoadOnly = false;
    };

    struct MovementAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string description;
        MovementAdjustmentType type;
        MovementSelector selector;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
    };

    class MovementAdjustment final
    {
    public:
        explicit MovementAdjustment(MovementAdjustmentDefinition definition);

    private:
        friend class Movement;

        std::string id_;
        std::string source_;
        std::string description_;
        MovementAdjustmentType type_;
        MovementSelector selector_;
        std::optional<std::string> expression_;
        std::optional<std::string> condition_;
    };

    struct RunAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string description;
        RunAdjustmentType type;
        std::string stackingGroup;
        MovementSelector selector;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
    };

    class RunAdjustment final
    {
    public:
        explicit RunAdjustment(RunAdjustmentDefinition definition);

    private:
        friend class Movement;

        std::string id_;
        std::string source_;
        std::string description_;
        RunAdjustmentType type_;
        std::string stackingGroup_;
        MovementSelector selector_;
        std::optional<std::string> expression_;
        std::optional<std::string> condition_;
    };

    class Movement final
    {
    public:
        explicit Movement(ResourceManager &resourceManager);

        MovementView toView();

    private:
        void addGrant(MovementGrant grant);
        void removeGrant(std::string_view grantId);
        void addAdjustment(MovementAdjustment adjustment);
        void removeAdjustment(std::string_view adjustmentId);
        void addRunAdjustment(RunAdjustment adjustment);
        void removeRunAdjustment(std::string_view adjustmentId);
        bool adjustmentMatches(const MovementAdjustment &adjustment, const MovementGrant &grant) const;
        bool runAdjustmentMatches(const RunAdjustment &adjustment, const MovementGrant &grant) const;

        ResourceManager &resourceManager_;
        std::vector<MovementGrant> grants_;
        std::vector<MovementAdjustment> adjustments_;
        std::map<std::string, RunAdjustment> runAdjustments_;
    };
}
